#pragma once
#define VPROF_MAIN()             \
    g_profCurrentProfile.main(); \
    VPROF("Main")

#define VPROF(name) VPROF_(name, VPROF_BUDGETGROUP_ROOT)
#define VPROF_(name, group) ProfilerScope Prof_(name, group);

#define VPROF_BUDGET(name, group) VPROF_(name, group)

#define VPROF_SCOPE_BEGIN(name) \
    do {                        \
    VPROF(name)
#define VPROF_SCOPE_END() \
    }                     \
    while(0)

#define VPROF_ENTER_SCOPE(name) g_profCurrentProfile.enterScope(name, VPROF_BUDGETGROUP_ROOT)
#define VPROF_EXIT_SCOPE() g_profCurrentProfile.exitScope()

#define VPROF_BUDGETGROUP_ROOT "Root"
#define VPROF_BUDGETGROUP_SLEEP "Sleep"
#define VPROF_BUDGETGROUP_WNDPROC "WndProc"
#define VPROF_BUDGETGROUP_UPDATE "Update"
#define VPROF_BUDGETGROUP_DRAW "Draw"
#define VPROF_BUDGETGROUP_DRAW_SWAPBUFFERS "SwapBuffers"

#define VPROF_MAX_NUM_BUDGETGROUPS 32
#define VPROF_MAX_NUM_NODES 32

class ProfilerNode {
    friend class ProfilerProfile;

   public:
    ProfilerNode();
    ProfilerNode(const char *name, const char *group, ProfilerNode *parent);

    void enterScope();
    bool exitScope();

    inline const char *getName() const { return this->name; }
    inline int getGroupID() const { return this->iGroupID; }

    inline ProfilerNode *getParent() const { return this->parent; }
    inline ProfilerNode *getChild() const { return this->child; }
    inline ProfilerNode *getSibling() const { return this->sibling; }

    inline double getTimeCurrentFrame() const {
        return this->fTimeCurrentFrame;
    }  // NOTE: this is incomplete if retrieved within engine update(), use getTimeLastFrame() instead
    inline double getTimeLastFrame() const { return this->fTimeLastFrame; }

   private:
    inline void constructor(const char *name, const char *group, ProfilerNode *parent);

    ProfilerNode *getSubNode(const char *name, const char *group);

    static int s_iNodeCounter;

    const char *name;
    int iGroupID;

    ProfilerNode *parent;
    ProfilerNode *child;
    ProfilerNode *sibling;

    int iNumRecursions;
    double fTime;
    double fTimeCurrentFrame;
    double fTimeLastFrame;
};

class ProfilerProfile {
    friend class ProfilerNode;

   public:
    ProfilerProfile(bool manualStartViaMain = false);

    inline void main() {
        if(this->bEnableScheduled) {
            this->bEnableScheduled = false;
            this->root.enterScope();
        }

        // collect all durations from the last frame and store them as a complete set
        if(this->iEnabled > 0) {
            for(int i = 0; i < this->iNumNodes; i++) {
                this->nodes[i].fTimeLastFrame = this->nodes[i].fTimeCurrentFrame;
            }
        }
    }

    inline void start() {
        if(++this->iEnabled == 1) {
            if(this->bManualStartViaMain)
                this->bEnableScheduled = true;
            else
                this->root.enterScope();
        }
    }

    inline void stop() {
        if(--this->iEnabled == 0) {
            if(!this->bEnableScheduled) this->root.exitScope();

            this->bEnableScheduled = false;
        }
    }

    inline void enterScope(const char *name, const char *group) {
        if((this->iEnabled != 0 && !this->bEnableScheduled) || !this->bAtRoot) {
            if(name != this->curNode->name)  // NOTE: pointer comparison
                this->curNode = this->curNode->getSubNode(name, group);

            this->curNode->enterScope();

            this->bAtRoot = (this->curNode == &this->root);
        }
    }

    inline void exitScope() {
        if(!this->bAtRoot || (this->iEnabled != 0 && !this->bEnableScheduled)) {
            if(!this->bAtRoot && this->curNode->exitScope()) this->curNode = this->curNode->parent;

            this->bAtRoot = (this->curNode == &this->root);
        }
    }

    inline bool isEnabled() const { return (this->iEnabled != 0 || this->bEnableScheduled); }
    inline bool isAtRoot() const { return this->bAtRoot; }

    inline int getNumGroups() const { return this->iNumGroups; }
    inline int getNumNodes() const { return this->iNumNodes; }

    inline const ProfilerNode *getRoot() const { return &this->root; }

    inline const char *getGroupName(int groupID) {
        return this->groups[groupID < 0 ? 0 : (groupID > this->iNumGroups - 1 ? this->iNumGroups - 1 : groupID)].name;
    }

    double sumTimes(int groupID);
    double sumTimes(ProfilerNode *node, int groupID);

   private:
    struct BUDGETGROUP {
        const char *name;
    };

    int groupNameToID(const char *group);

    int iNumGroups;
    BUDGETGROUP groups[VPROF_MAX_NUM_BUDGETGROUPS];

    bool bManualStartViaMain;

    int iEnabled;
    bool bEnableScheduled;
    bool bAtRoot;
    ProfilerNode root;
    ProfilerNode *curNode;

    int iNumNodes;
    ProfilerNode nodes[VPROF_MAX_NUM_NODES];
};

extern ProfilerProfile g_profCurrentProfile;

class ProfilerScope {
   public:
    inline ProfilerScope(const char *name, const char *group) { g_profCurrentProfile.enterScope(name, group); }
    inline ~ProfilerScope() { g_profCurrentProfile.exitScope(); }
};

extern ProfilerProfile g_profCurrentProfile;
