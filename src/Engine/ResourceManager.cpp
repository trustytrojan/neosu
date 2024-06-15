#include "ResourceManager.h"

#include <mutex>
#include <thread>

#include "ConVar.h"
#include "Engine.h"
#include "Environment.h"
#include "Timer.h"

using namespace std;

static std::mutex g_resourceManagerMutex;             // internal lock for nested async loads
static std::mutex g_resourceManagerLoadingWorkMutex;  // work vector lock across all threads

static void *_resourceLoaderThread(void *data);

class ResourceManagerLoaderThread {
   public:
    // self
    std::thread thread;

    // wait lock
    std::mutex loadingMutex;

    // args
    std::atomic<size_t> threadIndex;
    std::atomic<bool> running;
    std::vector<ResourceManager::LOADING_WORK> *loadingWork;
};

ConVar rm_numthreads(
    "rm_numthreads", 3, FCVAR_DEFAULT,
    "how many parallel resource loader threads are spawned once on startup (!), and subsequently used during runtime");
ConVar rm_warnings("rm_warnings", false, FCVAR_DEFAULT);
ConVar rm_debug_async_delay("rm_debug_async_delay", 0.0f, FCVAR_LOCKED);
ConVar rm_interrupt_on_destroy("rm_interrupt_on_destroy", true, FCVAR_LOCKED);
ConVar debug_rm_("debug_rm", false, FCVAR_DEFAULT);

ConVar *ResourceManager::debug_rm = &debug_rm_;

const char *ResourceManager::PATH_DEFAULT_IMAGES = MCENGINE_DATA_DIR "materials/";
const char *ResourceManager::PATH_DEFAULT_FONTS = MCENGINE_DATA_DIR "fonts/";
const char *ResourceManager::PATH_DEFAULT_SHADERS = MCENGINE_DATA_DIR "shaders/";

ResourceManager::ResourceManager() {
    m_bNextLoadAsync = false;
    m_iNumResourceInitPerFrameLimit = 1;

    m_loadingWork.reserve(32);

    // create loader threads
    for(int i = 0; i < rm_numthreads.getInt(); i++) {
        ResourceManagerLoaderThread *loaderThread = new ResourceManagerLoaderThread();

        loaderThread->loadingMutex.lock();  // stop loader thread immediately, wait for work
        loaderThread->threadIndex = i;
        loaderThread->running = true;
        loaderThread->loadingWork = &m_loadingWork;

        loaderThread->thread = std::thread(_resourceLoaderThread, (void *)loaderThread);
        loaderThread->thread.detach();
        m_threads.push_back(loaderThread);
    }
}

ResourceManager::~ResourceManager() {
    // release all not-currently-being-loaded resources (1)
    destroyResources();

    // let all loader threads exit
    for(size_t i = 0; i < m_threads.size(); i++) {
        m_threads[i]->running = false;
    }

    for(size_t i = 0; i < m_threads.size(); i++) {
        const size_t threadIndex = m_threads[i]->threadIndex.load();

        bool hasLoadingWork = false;
        for(size_t w = 0; w < m_loadingWork.size(); w++) {
            if(m_loadingWork[w].threadIndex.atomic.load() == threadIndex) {
                hasLoadingWork = true;
                break;
            }
        }

        if(!hasLoadingWork) m_threads[i]->loadingMutex.unlock();
    }

    // wait for threads to stop
    for(size_t i = 0; i < m_threads.size(); i++) {
        m_threads[i]->thread.join();
    }

    m_threads.clear();

    // cleanup leftovers (can only do that after loader threads have exited) (2)
    for(size_t i = 0; i < m_loadingWorkAsyncDestroy.size(); i++) {
        delete m_loadingWorkAsyncDestroy[i];
    }
    m_loadingWorkAsyncDestroy.clear();
}

void ResourceManager::update() {
    bool reLock = false;
    g_resourceManagerMutex.lock();
    {
        // handle load finish (and synchronous init())
        size_t numResourceInitCounter = 0;
        for(size_t i = 0; i < m_loadingWork.size(); i++) {
            if(m_loadingWork[i].done.atomic.load()) {
                if(debug_rm->getBool()) debugLog("Resource Manager: Worker thread #%i finished.\n", i);

                // copy pointer, so we can stop everything before finishing
                Resource *rs = m_loadingWork[i].resource.atomic.load();
                const size_t threadIndex = m_loadingWork[i].threadIndex.atomic.load();

                g_resourceManagerLoadingWorkMutex.lock();
                { m_loadingWork.erase(m_loadingWork.begin() + i); }
                g_resourceManagerLoadingWorkMutex.unlock();

                i--;

                // stop this worker thread if everything has been loaded
                int numLoadingWorkForThreadIndex = 0;
                for(size_t w = 0; w < m_loadingWork.size(); w++) {
                    if(m_loadingWork[w].threadIndex.atomic.load() == threadIndex) numLoadingWorkForThreadIndex++;
                }

                if(numLoadingWorkForThreadIndex < 1) {
                    if(m_threads.size() > 0) m_threads[threadIndex]->loadingMutex.lock();
                }

                // unlock. this allows resources to trigger "recursive" loads within init()
                g_resourceManagerMutex.unlock();
                reLock = true;

                // check if this was an async destroy, can skip load() if that is the case
                // TODO: this will probably break stuff, needs in depth testing before change, think more about this
                /*
                bool isAsyncDestroy = false;
                for (size_t a=0; a<m_loadingWorkAsyncDestroy.size(); a++)
                {
                        if (m_loadingWorkAsyncDestroy[a] == rs)
                        {
                                isAsyncDestroy = true;
                                break;
                        }
                }
                */

                // finish (synchronous init())
                // if (!isAsyncDestroy)

                rs->load();
                numResourceInitCounter++;

                // else if (debug_rm->getBool())
                //	debugLog("Resource Manager: Skipping load() due to async destroy of #%i\n", (i + 1));

                if(m_iNumResourceInitPerFrameLimit > 0 && numResourceInitCounter >= m_iNumResourceInitPerFrameLimit)
                    break;  // NOTE: only allow 1 work item to finish per frame (avoid stutters for e.g. texture
                            // uploads)
                else {
                    if(reLock) {
                        reLock = false;
                        g_resourceManagerMutex.lock();
                    }
                }
            }
        }

        if(reLock) {
            g_resourceManagerMutex.lock();
        }

        // handle async destroy
        for(size_t i = 0; i < m_loadingWorkAsyncDestroy.size(); i++) {
            bool canBeDestroyed = true;
            for(size_t w = 0; w < m_loadingWork.size(); w++) {
                if(m_loadingWork[w].resource.atomic.load() == m_loadingWorkAsyncDestroy[i]) {
                    if(debug_rm->getBool()) debugLog("Resource Manager: Waiting for async destroy of #%i ...\n", i);

                    canBeDestroyed = false;
                    break;
                }
            }

            if(canBeDestroyed) {
                if(debug_rm->getBool()) debugLog("Resource Manager: Async destroy of #%i\n", i);

                delete m_loadingWorkAsyncDestroy[i];  // implicitly calls release() through the Resource destructor
                m_loadingWorkAsyncDestroy.erase(m_loadingWorkAsyncDestroy.begin() + i);
                i--;
            }
        }
    }
    g_resourceManagerMutex.unlock();
}

void ResourceManager::destroyResources() {
    while(m_vResources.size() > 0) {
        destroyResource(m_vResources[0]);
    }
    m_vResources.clear();
}

void ResourceManager::destroyResource(Resource *rs) {
    if(rs == NULL) {
        if(rm_warnings.getBool()) debugLog("RESOURCE MANAGER Warning: destroyResource(NULL)!\n");
        return;
    }

    if(debug_rm->getBool()) debugLog("ResourceManager: Destroying %s\n", rs->getName().c_str());

    g_resourceManagerMutex.lock();
    {
        bool isManagedResource = false;
        int managedResourceIndex = -1;
        for(size_t i = 0; i < m_vResources.size(); i++) {
            if(m_vResources[i] == rs) {
                isManagedResource = true;
                managedResourceIndex = i;
                break;
            }
        }

        // handle async destroy
        for(size_t w = 0; w < m_loadingWork.size(); w++) {
            if(m_loadingWork[w].resource.atomic.load() == rs) {
                if(debug_rm->getBool())
                    debugLog("Resource Manager: Scheduled async destroy of %s\n", rs->getName().c_str());

                if(rm_interrupt_on_destroy.getBool()) rs->interruptLoad();

                m_loadingWorkAsyncDestroy.push_back(rs);
                if(isManagedResource) m_vResources.erase(m_vResources.begin() + managedResourceIndex);

                // NOTE: ugly
                g_resourceManagerMutex.unlock();
                return;  // we're done here
            }
        }

        // standard destroy
        SAFE_DELETE(rs);

        if(isManagedResource) m_vResources.erase(m_vResources.begin() + managedResourceIndex);
    }
    g_resourceManagerMutex.unlock();
}

void ResourceManager::reloadResources() {
    for(size_t i = 0; i < m_vResources.size(); i++) {
        m_vResources[i]->reload();
    }
}

void ResourceManager::requestNextLoadAsync() { m_bNextLoadAsync = true; }

void ResourceManager::requestNextLoadUnmanaged() { m_nextLoadUnmanagedStack.push(true); }

Image *ResourceManager::loadImage(std::string filepath, std::string resourceName, bool mipmapped) {
    // check if it already exists
    if(resourceName.length() > 0) {
        Resource *temp = checkIfExistsAndHandle(resourceName);
        if(temp != NULL) return dynamic_cast<Image *>(temp);
    }

    // create instance and load it
    filepath.insert(0, PATH_DEFAULT_IMAGES);
    Image *img = engine->getGraphics()->createImage(filepath, mipmapped);
    img->setName(resourceName);

    loadResource(img, true);

    return img;
}

Image *ResourceManager::loadImageAbs(std::string absoluteFilepath, std::string resourceName, bool mipmapped) {
    // check if it already exists
    if(resourceName.length() > 0) {
        Resource *temp = checkIfExistsAndHandle(resourceName);
        if(temp != NULL) return dynamic_cast<Image *>(temp);
    }

    // create instance and load it
    Image *img = engine->getGraphics()->createImage(absoluteFilepath, mipmapped);
    img->setName(resourceName);

    loadResource(img, true);

    return img;
}

Image *ResourceManager::loadImageAbsUnnamed(std::string absoluteFilepath, bool mipmapped) {
    Image *img = engine->getGraphics()->createImage(absoluteFilepath, mipmapped);

    loadResource(img, true);

    return img;
}

Image *ResourceManager::createImage(unsigned int width, unsigned int height, bool mipmapped) {
    if(width > 8192 || height > 8192) {
        engine->showMessageError(
            "Resource Manager Error",
            UString::format("Invalid parameters in createImage(%i, %i, %i)!\n", width, height, (int)mipmapped));
        return NULL;
    }

    Image *img = engine->getGraphics()->createImage(width, height, mipmapped);

    loadResource(img, false);

    return img;
}

McFont *ResourceManager::loadFont(std::string filepath, std::string resourceName, int fontSize, bool antialiasing,
                                  int fontDPI) {
    // check if it already exists
    if(resourceName.length() > 0) {
        Resource *temp = checkIfExistsAndHandle(resourceName);
        if(temp != NULL) return dynamic_cast<McFont *>(temp);
    }

    // create instance and load it
    filepath.insert(0, PATH_DEFAULT_FONTS);
    McFont *fnt = new McFont(filepath, fontSize, antialiasing, fontDPI);
    fnt->setName(resourceName);

    loadResource(fnt, true);

    return fnt;
}

McFont *ResourceManager::loadFont(std::string filepath, std::string resourceName, std::vector<wchar_t> characters,
                                  int fontSize, bool antialiasing, int fontDPI) {
    // check if it already exists
    if(resourceName.length() > 0) {
        Resource *temp = checkIfExistsAndHandle(resourceName);
        if(temp != NULL) return dynamic_cast<McFont *>(temp);
    }

    // create instance and load it
    filepath.insert(0, PATH_DEFAULT_FONTS);
    McFont *fnt = new McFont(filepath, characters, fontSize, antialiasing, fontDPI);
    fnt->setName(resourceName);

    loadResource(fnt, true);

    return fnt;
}

Sound *ResourceManager::loadSoundAbs(std::string filepath, std::string resourceName, bool stream, bool overlayable,
                                     bool loop) {
    // check if it already exists
    if(resourceName.length() > 0) {
        Resource *temp = checkIfExistsAndHandle(resourceName);
        if(temp != NULL) return dynamic_cast<Sound *>(temp);
    }

    // create instance and load it
    Sound *snd = new Sound(filepath, stream, overlayable, loop);
    snd->setName(resourceName);

    loadResource(snd, true);

    return snd;
}

Shader *ResourceManager::loadShader(std::string vertexShaderFilePath, std::string fragmentShaderFilePath,
                                    std::string resourceName) {
    // check if it already exists
    if(resourceName.length() > 0) {
        Resource *temp = checkIfExistsAndHandle(resourceName);
        if(temp != NULL) return dynamic_cast<Shader *>(temp);
    }

    // create instance and load it
    vertexShaderFilePath.insert(0, PATH_DEFAULT_SHADERS);
    fragmentShaderFilePath.insert(0, PATH_DEFAULT_SHADERS);
    Shader *shader = engine->getGraphics()->createShaderFromFile(vertexShaderFilePath, fragmentShaderFilePath);
    shader->setName(resourceName);

    loadResource(shader, true);

    return shader;
}

Shader *ResourceManager::loadShader(std::string vertexShaderFilePath, std::string fragmentShaderFilePath) {
    vertexShaderFilePath.insert(0, PATH_DEFAULT_SHADERS);
    fragmentShaderFilePath.insert(0, PATH_DEFAULT_SHADERS);
    Shader *shader = engine->getGraphics()->createShaderFromFile(vertexShaderFilePath, fragmentShaderFilePath);

    loadResource(shader, true);

    return shader;
}

Shader *ResourceManager::createShader(std::string vertexShader, std::string fragmentShader, std::string resourceName) {
    // check if it already exists
    if(resourceName.length() > 0) {
        Resource *temp = checkIfExistsAndHandle(resourceName);
        if(temp != NULL) return dynamic_cast<Shader *>(temp);
    }

    // create instance and load it
    Shader *shader = engine->getGraphics()->createShaderFromSource(vertexShader, fragmentShader);
    shader->setName(resourceName);

    loadResource(shader, true);

    return shader;
}

Shader *ResourceManager::createShader(std::string vertexShader, std::string fragmentShader) {
    Shader *shader = engine->getGraphics()->createShaderFromSource(vertexShader, fragmentShader);

    loadResource(shader, true);

    return shader;
}

RenderTarget *ResourceManager::createRenderTarget(int x, int y, int width, int height,
                                                  Graphics::MULTISAMPLE_TYPE multiSampleType) {
    RenderTarget *rt = engine->getGraphics()->createRenderTarget(x, y, width, height, multiSampleType);
    auto name = UString::format("_RT_%ix%i", width, height);
    rt->setName(name.toUtf8());

    loadResource(rt, true);

    return rt;
}

RenderTarget *ResourceManager::createRenderTarget(int width, int height, Graphics::MULTISAMPLE_TYPE multiSampleType) {
    return createRenderTarget(0, 0, width, height, multiSampleType);
}

TextureAtlas *ResourceManager::createTextureAtlas(int width, int height) {
    TextureAtlas *ta = new TextureAtlas(width, height);
    auto name = UString::format("_TA_%ix%i", width, height);
    ta->setName(name.toUtf8());

    loadResource(ta, false);

    return ta;
}

VertexArrayObject *ResourceManager::createVertexArrayObject(Graphics::PRIMITIVE primitive, Graphics::USAGE_TYPE usage,
                                                            bool keepInSystemMemory) {
    VertexArrayObject *vao = engine->getGraphics()->createVertexArrayObject(primitive, usage, keepInSystemMemory);

    loadResource(vao, false);

    return vao;
}

Image *ResourceManager::getImage(std::string resourceName) const {
    for(size_t i = 0; i < m_vResources.size(); i++) {
        if(m_vResources[i]->getName() == resourceName) return dynamic_cast<Image *>(m_vResources[i]);
    }

    doesntExistWarning(resourceName);
    return NULL;
}

McFont *ResourceManager::getFont(std::string resourceName) const {
    for(size_t i = 0; i < m_vResources.size(); i++) {
        if(m_vResources[i]->getName() == resourceName) return dynamic_cast<McFont *>(m_vResources[i]);
    }

    doesntExistWarning(resourceName);
    return NULL;
}

Sound *ResourceManager::getSound(std::string resourceName) const {
    for(size_t i = 0; i < m_vResources.size(); i++) {
        if(m_vResources[i]->getName() == resourceName) return dynamic_cast<Sound *>(m_vResources[i]);
    }

    doesntExistWarning(resourceName);
    return NULL;
}

Shader *ResourceManager::getShader(std::string resourceName) const {
    for(size_t i = 0; i < m_vResources.size(); i++) {
        if(m_vResources[i]->getName() == resourceName) return dynamic_cast<Shader *>(m_vResources[i]);
    }

    doesntExistWarning(resourceName);
    return NULL;
}

bool ResourceManager::isLoading() const { return (m_loadingWork.size() > 0); }

bool ResourceManager::isLoadingResource(Resource *rs) const {
    for(size_t i = 0; i < m_loadingWork.size(); i++) {
        if(m_loadingWork[i].resource.atomic.load() == rs) return true;
    }

    return false;
}

void ResourceManager::loadResource(Resource *res, bool load) {
    // handle flags
    if(m_nextLoadUnmanagedStack.size() < 1 || !m_nextLoadUnmanagedStack.top())
        m_vResources.push_back(res);  // add managed resource

    const bool isNextLoadAsync = m_bNextLoadAsync;

    // flags must be reset on every load, to not carry over
    resetFlags();

    if(!load) return;

    if(!isNextLoadAsync) {
        // load normally
        res->loadAsync();
        res->load();
    } else {
        if(rm_numthreads.getInt() > 0) {
            g_resourceManagerMutex.lock();
            {
                // TODO: prefer thread which currently doesn't have anything to do (i.e. allow n-1 "permanent"
                // background tasks without blocking)

                // split work evenly/linearly across all threads
                static size_t threadIndexCounter = 0;
                const size_t threadIndex = threadIndexCounter;

                // add work to loading thread
                LOADING_WORK work;

                work.resource = MobileAtomicResource(res);
                work.threadIndex = MobileAtomicSizeT(threadIndex);
                work.done = MobileAtomicBool(false);

                threadIndexCounter =
                    (threadIndexCounter + 1) % (min(m_threads.size(), (size_t)max(rm_numthreads.getInt(), 1)));

                g_resourceManagerLoadingWorkMutex.lock();
                {
                    m_loadingWork.push_back(work);

                    int numLoadingWorkForThreadIndex = 0;
                    for(size_t i = 0; i < m_loadingWork.size(); i++) {
                        if(m_loadingWork[i].threadIndex.atomic.load() == threadIndex) numLoadingWorkForThreadIndex++;
                    }

                    // let the loading thread run
                    if(numLoadingWorkForThreadIndex == 1)  // only necessary if thread is waiting (otherwise it will
                                                           // already be picked up by the next iteration)
                    {
                        if(m_threads.size() > 0) m_threads[threadIndex]->loadingMutex.unlock();
                    }
                }
                g_resourceManagerLoadingWorkMutex.unlock();
            }
            g_resourceManagerMutex.unlock();
        } else {
            // load normally (threading disabled)
            res->loadAsync();
            res->load();
        }
    }
}

void ResourceManager::doesntExistWarning(std::string resourceName) const {
    if(rm_warnings.getBool()) {
        UString errormsg = "Resource \"";
        errormsg.append(resourceName.c_str());
        errormsg.append("\" does not exist!");
        engine->showMessageWarning("RESOURCE MANAGER: ", errormsg);
    }
}

Resource *ResourceManager::checkIfExistsAndHandle(std::string resourceName) {
    for(size_t i = 0; i < m_vResources.size(); i++) {
        if(m_vResources[i]->getName() == resourceName) {
            if(rm_warnings.getBool())
                debugLog("RESOURCE MANAGER: Resource \"%s\" already loaded!\n", resourceName.c_str());

            // handle flags (reset them)
            resetFlags();

            return m_vResources[i];
        }
    }

    return NULL;
}

void ResourceManager::resetFlags() {
    if(m_nextLoadUnmanagedStack.size() > 0) m_nextLoadUnmanagedStack.pop();

    m_bNextLoadAsync = false;
}

static void *_resourceLoaderThread(void *data) {
    ResourceManagerLoaderThread *self = (ResourceManagerLoaderThread *)data;

    const size_t threadIndex = self->threadIndex.load();

    while(self->running.load()) {
        // wait for work
        self->loadingMutex.lock();  // thread will wait here if locked by engine
        self->loadingMutex.unlock();

        Resource *resourceToLoad = NULL;

        // quickly check if there is work to do (this can potentially cause engine lag!)
        // NOTE: we can't keep references to shared loadingWork objects (vector realloc/erase/etc.)
        g_resourceManagerLoadingWorkMutex.lock();
        {
            for(size_t i = 0; i < self->loadingWork->size(); i++) {
                if((*self->loadingWork)[i].threadIndex.atomic.load() == threadIndex &&
                   !(*self->loadingWork)[i].done.atomic.load()) {
                    resourceToLoad = (*self->loadingWork)[i].resource.atomic.load();
                    break;
                }
            }
        }
        g_resourceManagerLoadingWorkMutex.unlock();

        // if we have work
        if(resourceToLoad != NULL) {
            // debug
            if(rm_debug_async_delay.getFloat() > 0.0f) env->sleep(rm_debug_async_delay.getFloat() * 1000 * 1000);

            // asynchronous initAsync()
            resourceToLoad->loadAsync();

            // very quickly signal that we are done
            g_resourceManagerLoadingWorkMutex.lock();
            {
                for(size_t i = 0; i < self->loadingWork->size(); i++) {
                    if((*self->loadingWork)[i].threadIndex.atomic.load() == threadIndex &&
                       (*self->loadingWork)[i].resource.atomic.load() == resourceToLoad) {
                        (*self->loadingWork)[i].done = ResourceManager::MobileAtomicBool(true);
                        break;
                    }
                }
            }
            g_resourceManagerLoadingWorkMutex.unlock();
        } else
            env->sleep(1000);  // 1000 Hz sanity limit until locked again
    }

    return NULL;
}
