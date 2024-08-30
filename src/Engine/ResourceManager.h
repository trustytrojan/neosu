#pragma once
#include "Font.h"
#include "Image.h"
#include "RenderTarget.h"
#include "Shader.h"
#include "Sound.h"
#include "TextureAtlas.h"
#include "VertexArrayObject.h"

class ConVar;

class ResourceManagerLoaderThread;

class ResourceManager {
   public:
    static const char *PATH_DEFAULT_IMAGES;
    static const char *PATH_DEFAULT_FONTS;
    static const char *PATH_DEFAULT_SHADERS;

   public:
    template <typename T>
    struct MobileAtomic {
        std::atomic<T> atomic;

        MobileAtomic() : atomic(T()) {}

        explicit MobileAtomic(T const &v) : atomic(v) {}
        explicit MobileAtomic(std::atomic<T> const &a) : atomic(a.load()) {}

        MobileAtomic(MobileAtomic const &other) : atomic(other.atomic.load()) {}

        MobileAtomic &operator=(MobileAtomic const &other) {
            atomic.store(other.atomic.load());
            return *this;
        }
    };
    typedef MobileAtomic<bool> MobileAtomicBool;
    typedef MobileAtomic<size_t> MobileAtomicSizeT;
    typedef MobileAtomic<Resource *> MobileAtomicResource;

    struct LOADING_WORK {
        MobileAtomicResource resource;
        MobileAtomicSizeT threadIndex;
        MobileAtomicBool done;
    };

   public:
    ResourceManager();
    ~ResourceManager();

    void update();

    void setNumResourceInitPerFrameLimit(size_t numResourceInitPerFrameLimit) {
        m_iNumResourceInitPerFrameLimit = numResourceInitPerFrameLimit;
    }

    void loadResource(Resource *rs) {
        requestNextLoadUnmanaged();
        loadResource(rs, true);
    }
    void destroyResource(Resource *rs);
    void destroyResources();
    void reloadResources();

    void requestNextLoadAsync();
    void requestNextLoadUnmanaged();

    // images
    Image *loadImage(std::string filepath, std::string resourceName, bool mipmapped = false);
    Image *loadImageAbs(std::string absoluteFilepath, std::string resourceName, bool mipmapped = false);
    Image *loadImageAbsUnnamed(std::string absoluteFilepath, bool mipmapped = false);
    Image *createImage(unsigned int width, unsigned int height, bool mipmapped = false);

    // fonts
    McFont *loadFont(std::string filepath, std::string resourceName, int fontSize = 16, bool antialiasing = true,
                     int fontDPI = 96);
    McFont *loadFont(std::string filepath, std::string resourceName, std::vector<wchar_t> characters, int fontSize = 16,
                     bool antialiasing = true, int fontDPI = 96);

    // sounds
    Sound *loadSoundAbs(std::string filepath, std::string resourceName, bool stream = false, bool overlayable = false,
                        bool loop = false);

    // shaders
    Shader *loadShader(std::string vertexShaderFilePath, std::string fragmentShaderFilePath, std::string resourceName);
    Shader *loadShader(std::string vertexShaderFilePath, std::string fragmentShaderFilePath);
    Shader *createShader(std::string vertexShader, std::string fragmentShader, std::string resourceName);
    Shader *createShader(std::string vertexShader, std::string fragmentShader);

    // rendertargets
    RenderTarget *createRenderTarget(
        int x, int y, int width, int height,
        Graphics::MULTISAMPLE_TYPE multiSampleType = Graphics::MULTISAMPLE_TYPE::MULTISAMPLE_0X);
    RenderTarget *createRenderTarget(
        int width, int height, Graphics::MULTISAMPLE_TYPE multiSampleType = Graphics::MULTISAMPLE_TYPE::MULTISAMPLE_0X);

    // texture atlas
    TextureAtlas *createTextureAtlas(int width, int height);

    // models/meshes
    VertexArrayObject *createVertexArrayObject(Graphics::PRIMITIVE primitive = Graphics::PRIMITIVE::PRIMITIVE_TRIANGLES,
                                               Graphics::USAGE_TYPE usage = Graphics::USAGE_TYPE::USAGE_STATIC,
                                               bool keepInSystemMemory = false);

    // resource access by name
    Image *getImage(std::string resourceName) const;
    McFont *getFont(std::string resourceName) const;
    Sound *getSound(std::string resourceName) const;
    Shader *getShader(std::string resourceName) const;

    inline const std::vector<Resource *> &getResources() const { return m_vResources; }
    inline size_t getNumThreads() const { return m_threads.size(); }
    inline size_t getNumLoadingWork() const { return m_loadingWork.size(); }
    inline size_t getNumLoadingWorkAsyncDestroy() const { return m_loadingWorkAsyncDestroy.size(); }

    bool isLoading() const;
    bool isLoadingResource(Resource *rs) const;

   private:
    void loadResource(Resource *res, bool load);
    void doesntExistWarning(std::string resourceName) const;
    Resource *checkIfExistsAndHandle(std::string resourceName);

    void resetFlags();

    // content
    std::vector<Resource *> m_vResources;

    // flags
    bool m_bNextLoadAsync;
    std::stack<bool> m_nextLoadUnmanagedStack;
    size_t m_iNumResourceInitPerFrameLimit;

    // async
    std::vector<ResourceManagerLoaderThread *> m_threads;
    std::vector<LOADING_WORK> m_loadingWork;
    std::vector<Resource *> m_loadingWorkAsyncDestroy;
};
