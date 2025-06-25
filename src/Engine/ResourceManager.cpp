#include "ResourceManager.h"

#include <mutex>
#include <thread>

#include "ConVar.h"
#include "Engine.h"
#include "Environment.h"
#include "Timing.h"

using namespace std;

static std::mutex g_resourceManagerMutex;             // internal lock for nested async loads
static std::mutex g_resourceManagerLoadingWorkMutex;  // work vector lock across all threads

static void *_resourceLoaderThread(void *data);

class ResourceManagerLoaderThread {
   public:
    ~ResourceManagerLoaderThread() {
        this->running = false;
        this->loadingMutex.unlock();
        this->thread.join();
    }

    // self
    std::thread thread;

    // wait lock
    std::mutex loadingMutex;

    // args
    std::atomic<size_t> threadIndex;
    std::atomic<bool> running;
    std::vector<ResourceManager::LOADING_WORK> *loadingWork;
};

const char *ResourceManager::PATH_DEFAULT_IMAGES = MCENGINE_DATA_DIR "materials/";
const char *ResourceManager::PATH_DEFAULT_FONTS = MCENGINE_DATA_DIR "fonts/";
const char *ResourceManager::PATH_DEFAULT_SHADERS = MCENGINE_DATA_DIR "shaders/";

ResourceManager::ResourceManager() {
    this->bNextLoadAsync = false;
    this->iNumResourceInitPerFrameLimit = 1;

    this->loadingWork.reserve(32);

    // create loader threads
    for(int i = 0; i < cv_rm_numthreads.getInt(); i++) {
        ResourceManagerLoaderThread *loaderThread = new ResourceManagerLoaderThread();

        loaderThread->loadingMutex.lock();  // stop loader thread immediately, wait for work
        loaderThread->threadIndex = i;
        loaderThread->running = true;
        loaderThread->loadingWork = &this->loadingWork;

        loaderThread->thread = std::thread(_resourceLoaderThread, (void *)loaderThread);
        this->threads.push_back(loaderThread);
    }
}

ResourceManager::~ResourceManager() {
    // release all not-currently-being-loaded resources (1)
    this->destroyResources();

    for(auto thread : this->threads) {
        delete thread;
    }
    this->threads.clear();

    // cleanup leftovers (can only do that after loader threads have exited) (2)
    for(size_t i = 0; i < this->loadingWorkAsyncDestroy.size(); i++) {
        delete this->loadingWorkAsyncDestroy[i];
    }
    this->loadingWorkAsyncDestroy.clear();
}

void ResourceManager::update() {
    bool reLock = false;
    g_resourceManagerMutex.lock();
    {
        // handle load finish (and synchronous init())
        size_t numResourceInitCounter = 0;
        for(size_t i = 0; i < this->loadingWork.size(); i++) {
            if(this->loadingWork[i].done.atomic.load()) {
                if(cv_debug_rm.getBool()) debugLog("Resource Manager: Worker thread #%i finished.\n", i);

                // copy pointer, so we can stop everything before finishing
                Resource *rs = this->loadingWork[i].resource.atomic.load();
                const size_t threadIndex = this->loadingWork[i].threadIndex.atomic.load();

                g_resourceManagerLoadingWorkMutex.lock();
                { this->loadingWork.erase(this->loadingWork.begin() + i); }
                g_resourceManagerLoadingWorkMutex.unlock();

                i--;

                // stop this worker thread if everything has been loaded
                int numLoadingWorkForThreadIndex = 0;
                for(size_t w = 0; w < this->loadingWork.size(); w++) {
                    if(this->loadingWork[w].threadIndex.atomic.load() == threadIndex) numLoadingWorkForThreadIndex++;
                }

                if(numLoadingWorkForThreadIndex < 1) {
                    if(this->threads.size() > 0) this->threads[threadIndex]->loadingMutex.lock();
                }

                // unlock. this allows resources to trigger "recursive" loads within init()
                g_resourceManagerMutex.unlock();
                reLock = true;

                rs->load();
                numResourceInitCounter++;

                // else if (debug_rm->getBool())
                //	debugLog("Resource Manager: Skipping load() due to async destroy of #%i\n", (i + 1));

                if(this->iNumResourceInitPerFrameLimit > 0 &&
                   numResourceInitCounter >= this->iNumResourceInitPerFrameLimit)
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
        for(size_t i = 0; i < this->loadingWorkAsyncDestroy.size(); i++) {
            bool canBeDestroyed = true;
            for(size_t w = 0; w < this->loadingWork.size(); w++) {
                if(this->loadingWork[w].resource.atomic.load() == this->loadingWorkAsyncDestroy[i]) {
                    if(cv_debug_rm.getBool()) debugLog("Resource Manager: Waiting for async destroy of #%i ...\n", i);

                    canBeDestroyed = false;
                    break;
                }
            }

            if(canBeDestroyed) {
                if(cv_debug_rm.getBool()) debugLog("Resource Manager: Async destroy of #%i\n", i);

                delete this->loadingWorkAsyncDestroy[i];  // implicitly calls release() through the Resource destructor
                this->loadingWorkAsyncDestroy.erase(this->loadingWorkAsyncDestroy.begin() + i);
                i--;
            }
        }
    }
    g_resourceManagerMutex.unlock();
}

void ResourceManager::destroyResources() {
    while(this->vResources.size() > 0) {
        this->destroyResource(this->vResources[0]);
    }
    this->vResources.clear();
}

void ResourceManager::destroyResource(Resource *rs) {
    if(rs == NULL) {
        if(cv_rm_warnings.getBool()) debugLog("RESOURCE MANAGER Warning: destroyResource(NULL)!\n");
        return;
    }

    if(cv_debug_rm.getBool()) debugLog("ResourceManager: Destroying %s\n", rs->getName().c_str());

    g_resourceManagerMutex.lock();
    {
        bool isManagedResource = false;
        int managedResourceIndex = -1;
        for(size_t i = 0; i < this->vResources.size(); i++) {
            if(this->vResources[i] == rs) {
                isManagedResource = true;
                managedResourceIndex = i;
                break;
            }
        }

        // handle async destroy
        for(size_t w = 0; w < this->loadingWork.size(); w++) {
            if(this->loadingWork[w].resource.atomic.load() == rs) {
                if(cv_debug_rm.getBool())
                    debugLog("Resource Manager: Scheduled async destroy of %s\n", rs->getName().c_str());

                if(cv_rm_interrupt_on_destroy.getBool()) rs->interruptLoad();

                this->loadingWorkAsyncDestroy.push_back(rs);
                if(isManagedResource) this->vResources.erase(this->vResources.begin() + managedResourceIndex);

                // NOTE: ugly
                g_resourceManagerMutex.unlock();
                return;  // we're done here
            }
        }

        // standard destroy
        SAFE_DELETE(rs);

        if(isManagedResource) this->vResources.erase(this->vResources.begin() + managedResourceIndex);
    }
    g_resourceManagerMutex.unlock();
}

void ResourceManager::reloadResources() {
    for(size_t i = 0; i < this->vResources.size(); i++) {
        this->vResources[i]->reload();
    }
}

void ResourceManager::requestNextLoadAsync() { this->bNextLoadAsync = true; }

void ResourceManager::requestNextLoadUnmanaged() { this->nextLoadUnmanagedStack.push(true); }

Image *ResourceManager::loadImage(std::string filepath, std::string resourceName, bool mipmapped) {
    // check if it already exists
    if(resourceName.length() > 0) {
        Resource *temp = this->checkIfExistsAndHandle(resourceName);
        if(temp != NULL) return dynamic_cast<Image *>(temp);
    }

    // create instance and load it
    filepath.insert(0, PATH_DEFAULT_IMAGES);
    Image *img = engine->getGraphics()->createImage(filepath, mipmapped);
    img->setName(resourceName);

    this->loadResource(img, true);

    return img;
}

Image *ResourceManager::loadImageAbs(std::string absoluteFilepath, std::string resourceName, bool mipmapped) {
    // check if it already exists
    if(resourceName.length() > 0) {
        Resource *temp = this->checkIfExistsAndHandle(resourceName);
        if(temp != NULL) return dynamic_cast<Image *>(temp);
    }

    // create instance and load it
    Image *img = engine->getGraphics()->createImage(absoluteFilepath, mipmapped);
    img->setName(resourceName);

    this->loadResource(img, true);

    return img;
}

Image *ResourceManager::loadImageAbsUnnamed(std::string absoluteFilepath, bool mipmapped) {
    Image *img = engine->getGraphics()->createImage(absoluteFilepath, mipmapped);

    this->loadResource(img, true);

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

    this->loadResource(img, false);

    return img;
}

McFont *ResourceManager::loadFont(std::string filepath, std::string resourceName, int fontSize, bool antialiasing,
                                  int fontDPI) {
    // check if it already exists
    if(resourceName.length() > 0) {
        Resource *temp = this->checkIfExistsAndHandle(resourceName);
        if(temp != NULL) return dynamic_cast<McFont *>(temp);
    }

    // create instance and load it
    filepath.insert(0, PATH_DEFAULT_FONTS);
    McFont *fnt = new McFont(UString{filepath}, fontSize, antialiasing, fontDPI);
    fnt->setName(resourceName);

    this->loadResource(fnt, true);

    return fnt;
}

McFont *ResourceManager::loadFont(std::string filepath, std::string resourceName, std::vector<wchar_t> characters,
                                  int fontSize, bool antialiasing, int fontDPI) {
    // check if it already exists
    if(resourceName.length() > 0) {
        Resource *temp = this->checkIfExistsAndHandle(resourceName);
        if(temp != NULL) return dynamic_cast<McFont *>(temp);
    }

    // create instance and load it
    filepath.insert(0, PATH_DEFAULT_FONTS);
    McFont *fnt = new McFont(UString{filepath}, characters, fontSize, antialiasing, fontDPI);
    fnt->setName(resourceName);

    this->loadResource(fnt, true);

    return fnt;
}

Sound *ResourceManager::loadSoundAbs(std::string filepath, std::string resourceName, bool stream, bool overlayable,
                                     bool loop) {
    // check if it already exists
    if(resourceName.length() > 0) {
        Resource *temp = this->checkIfExistsAndHandle(resourceName);
        if(temp != NULL) return dynamic_cast<Sound *>(temp);
    }

    // create instance and load it
    Sound *snd = new Sound(filepath, stream, overlayable, loop);
    snd->setName(resourceName);

    this->loadResource(snd, true);

    return snd;
}

Shader *ResourceManager::loadShader(std::string vertexShaderFilePath, std::string fragmentShaderFilePath,
                                    std::string resourceName) {
    // check if it already exists
    if(resourceName.length() > 0) {
        Resource *temp = this->checkIfExistsAndHandle(resourceName);
        if(temp != NULL) return dynamic_cast<Shader *>(temp);
    }

    // create instance and load it
    vertexShaderFilePath.insert(0, PATH_DEFAULT_SHADERS);
    fragmentShaderFilePath.insert(0, PATH_DEFAULT_SHADERS);
    Shader *shader = engine->getGraphics()->createShaderFromFile(vertexShaderFilePath, fragmentShaderFilePath);
    shader->setName(resourceName);

    this->loadResource(shader, true);

    return shader;
}

Shader *ResourceManager::loadShader(std::string vertexShaderFilePath, std::string fragmentShaderFilePath) {
    vertexShaderFilePath.insert(0, PATH_DEFAULT_SHADERS);
    fragmentShaderFilePath.insert(0, PATH_DEFAULT_SHADERS);
    Shader *shader = engine->getGraphics()->createShaderFromFile(vertexShaderFilePath, fragmentShaderFilePath);

    this->loadResource(shader, true);

    return shader;
}

Shader *ResourceManager::createShader(std::string vertexShader, std::string fragmentShader, std::string resourceName) {
    // check if it already exists
    if(resourceName.length() > 0) {
        Resource *temp = this->checkIfExistsAndHandle(resourceName);
        if(temp != NULL) return dynamic_cast<Shader *>(temp);
    }

    // create instance and load it
    Shader *shader = engine->getGraphics()->createShaderFromSource(vertexShader, fragmentShader);
    shader->setName(resourceName);

    this->loadResource(shader, true);

    return shader;
}

Shader *ResourceManager::createShader(std::string vertexShader, std::string fragmentShader) {
    Shader *shader = engine->getGraphics()->createShaderFromSource(vertexShader, fragmentShader);

    this->loadResource(shader, true);

    return shader;
}

RenderTarget *ResourceManager::createRenderTarget(int x, int y, int width, int height,
                                                  Graphics::MULTISAMPLE_TYPE multiSampleType) {
    RenderTarget *rt = engine->getGraphics()->createRenderTarget(x, y, width, height, multiSampleType);
    auto name = UString::format("_RT_%ix%i", width, height);
    rt->setName(name.toUtf8());

    this->loadResource(rt, true);

    return rt;
}

RenderTarget *ResourceManager::createRenderTarget(int width, int height, Graphics::MULTISAMPLE_TYPE multiSampleType) {
    return this->createRenderTarget(0, 0, width, height, multiSampleType);
}

TextureAtlas *ResourceManager::createTextureAtlas(int width, int height) {
    TextureAtlas *ta = new TextureAtlas(width, height);
    auto name = UString::format("_TA_%ix%i", width, height);
    ta->setName(name.toUtf8());

    this->loadResource(ta, false);

    return ta;
}

VertexArrayObject *ResourceManager::createVertexArrayObject(Graphics::PRIMITIVE primitive, Graphics::USAGE_TYPE usage,
                                                            bool keepInSystemMemory) {
    VertexArrayObject *vao = engine->getGraphics()->createVertexArrayObject(primitive, usage, keepInSystemMemory);

    this->loadResource(vao, false);

    return vao;
}

Image *ResourceManager::getImage(std::string resourceName) const {
    for(size_t i = 0; i < this->vResources.size(); i++) {
        if(this->vResources[i]->getName() == resourceName) return dynamic_cast<Image *>(this->vResources[i]);
    }

    this->doesntExistWarning(resourceName);
    return NULL;
}

McFont *ResourceManager::getFont(std::string resourceName) const {
    for(size_t i = 0; i < this->vResources.size(); i++) {
        if(this->vResources[i]->getName() == resourceName) return dynamic_cast<McFont *>(this->vResources[i]);
    }

    this->doesntExistWarning(resourceName);
    return NULL;
}

Sound *ResourceManager::getSound(std::string resourceName) const {
    for(size_t i = 0; i < this->vResources.size(); i++) {
        if(this->vResources[i]->getName() == resourceName) return dynamic_cast<Sound *>(this->vResources[i]);
    }

    this->doesntExistWarning(resourceName);
    return NULL;
}

Shader *ResourceManager::getShader(std::string resourceName) const {
    for(size_t i = 0; i < this->vResources.size(); i++) {
        if(this->vResources[i]->getName() == resourceName) return dynamic_cast<Shader *>(this->vResources[i]);
    }

    this->doesntExistWarning(resourceName);
    return NULL;
}

bool ResourceManager::isLoading() const { return (this->loadingWork.size() > 0); }

bool ResourceManager::isLoadingResource(Resource *rs) const {
    for(size_t i = 0; i < this->loadingWork.size(); i++) {
        if(this->loadingWork[i].resource.atomic.load() == rs) return true;
    }

    return false;
}

void ResourceManager::loadResource(Resource *res, bool load) {
    // handle flags
    if(this->nextLoadUnmanagedStack.size() < 1 || !this->nextLoadUnmanagedStack.top())
        this->vResources.push_back(res);  // add managed resource

    const bool isNextLoadAsync = this->bNextLoadAsync;

    // flags must be reset on every load, to not carry over
    this->resetFlags();

    if(!load) return;

    if(!isNextLoadAsync) {
        // load normally
        res->loadAsync();
        res->load();
    } else {
        if(cv_rm_numthreads.getInt() > 0) {
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
                    (threadIndexCounter + 1) % (min(this->threads.size(), (size_t)max(cv_rm_numthreads.getInt(), 1)));

                g_resourceManagerLoadingWorkMutex.lock();
                {
                    this->loadingWork.push_back(work);

                    int numLoadingWorkForThreadIndex = 0;
                    for(size_t i = 0; i < this->loadingWork.size(); i++) {
                        if(this->loadingWork[i].threadIndex.atomic.load() == threadIndex)
                            numLoadingWorkForThreadIndex++;
                    }

                    // let the loading thread run
                    if(numLoadingWorkForThreadIndex == 1)  // only necessary if thread is waiting (otherwise it will
                                                           // already be picked up by the next iteration)
                    {
                        if(this->threads.size() > 0) this->threads[threadIndex]->loadingMutex.unlock();
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
    if(cv_rm_warnings.getBool()) {
        UString errormsg = "Resource \"";
        errormsg.append(resourceName.c_str());
        errormsg.append("\" does not exist!");
        engine->showMessageWarning("RESOURCE MANAGER: ", errormsg);
    }
}

Resource *ResourceManager::checkIfExistsAndHandle(std::string resourceName) {
    for(size_t i = 0; i < this->vResources.size(); i++) {
        if(this->vResources[i]->getName() == resourceName) {
            if(cv_rm_warnings.getBool())
                debugLog("RESOURCE MANAGER: Resource \"%s\" already loaded!\n", resourceName.c_str());

            // handle flags (reset them)
            this->resetFlags();

            return this->vResources[i];
        }
    }

    return NULL;
}

void ResourceManager::resetFlags() {
    if(this->nextLoadUnmanagedStack.size() > 0) this->nextLoadUnmanagedStack.pop();

    this->bNextLoadAsync = false;
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
            if(cv_rm_debug_async_delay.getFloat() > 0.0f) env->sleep(cv_rm_debug_async_delay.getFloat() * 1000 * 1000);

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
