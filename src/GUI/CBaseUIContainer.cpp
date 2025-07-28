#include "CBaseUIContainer.h"

#include <utility>

#include "Engine.h"

CBaseUIContainer::CBaseUIContainer(float Xpos, float Ypos, float Xsize, float Ysize, UString name)
    : CBaseUIElement(Xpos, Ypos, Xsize, Ysize, std::move(name)) {}

CBaseUIContainer::~CBaseUIContainer() { this->freeElements(); }

// free memory from children
void CBaseUIContainer::freeElements() {
    for(auto &elem : this->vElements) {
        SAFE_DELETE(elem);
    }
    this->vElements.clear();
}

// invalidate children without freeing memory
void CBaseUIContainer::invalidate() { this->vElements.clear(); }

CBaseUIContainer *CBaseUIContainer::addBaseUIElement(CBaseUIElement *element, float xPos, float yPos) {
    if(element == NULL) return this;

    element->setRelPos(xPos, yPos);
    element->setPos(this->vPos + element->getRelPos());
    this->vElements.push_back(element);

    return this;
}

CBaseUIContainer *CBaseUIContainer::addBaseUIElement(CBaseUIElement *element) {
    if(element == NULL) return this;

    element->setRelPos(element->getPos().x, element->getPos().y);
    element->setPos(this->vPos + element->getRelPos());
    this->vElements.push_back(element);

    return this;
}

CBaseUIContainer *CBaseUIContainer::addBaseUIElementBack(CBaseUIElement *element, float xPos, float yPos) {
    if(element == NULL) return this;

    element->setRelPos(xPos, yPos);
    element->setPos(this->vPos + element->getRelPos());
    this->vElements.insert(this->vElements.begin(), element);

    return this;
}

CBaseUIContainer *CBaseUIContainer::addBaseUIElementBack(CBaseUIElement *element) {
    if(element == NULL) return this;

    element->setRelPos(element->getPos().x, element->getPos().y);
    element->setPos(this->vPos + element->getRelPos());
    this->vElements.insert(this->vElements.begin(), element);

    return this;
}

CBaseUIContainer *CBaseUIContainer::insertBaseUIElement(CBaseUIElement *element, CBaseUIElement *index) {
    if(element == NULL || index == NULL) return this;

    element->setRelPos(element->getPos().x, element->getPos().y);
    element->setPos(this->vPos + element->getRelPos());
    for(size_t i = 0; i < this->vElements.size(); i++) {
        if(this->vElements[i] == index) {
            this->vElements.insert(this->vElements.begin() + std::clamp<int>(i, 0, this->vElements.size()), element);
            return this;
        }
    }

    debugLog("Warning: CBaseUIContainer::insertBaseUIElement() couldn't find index\n");

    return this;
}

CBaseUIContainer *CBaseUIContainer::insertBaseUIElementBack(CBaseUIElement *element, CBaseUIElement *index) {
    if(element == NULL || index == NULL) return this;

    element->setRelPos(element->getPos().x, element->getPos().y);
    element->setPos(this->vPos + element->getRelPos());
    for(size_t i = 0; i < this->vElements.size(); i++) {
        if(this->vElements[i] == index) {
            this->vElements.insert(this->vElements.begin() + std::clamp<int>(i + 1, 0, this->vElements.size()),
                                   element);
            return this;
        }
    }

    debugLog("Warning: CBaseUIContainer::insertBaseUIElementBack() couldn't find index\n");

    return this;
}

CBaseUIContainer *CBaseUIContainer::removeBaseUIElement(CBaseUIElement *element) {
    for(size_t i = 0; i < this->vElements.size(); i++) {
        if(this->vElements[i] == element) {
            this->vElements.erase(this->vElements.begin() + i);
            return this;
        }
    }

    debugLog("Warning: CBaseUIContainer::removeBaseUIElement() couldn't find element\n");

    return this;
}

CBaseUIContainer *CBaseUIContainer::deleteBaseUIElement(CBaseUIElement *element) {
    for(size_t i = 0; i < this->vElements.size(); i++) {
        if(this->vElements[i] == element) {
            delete element;
            this->vElements.erase(this->vElements.begin() + i);
            return this;
        }
    }

    debugLog("Warning: CBaseUIContainer::deleteBaseUIElement() couldn't find element\n");

    return this;
}

CBaseUIElement *CBaseUIContainer::getBaseUIElement(const UString &name) {
    MC_UNROLL
    for(auto &elem : this->vElements) {
        if(elem->getName() == name) return elem;
    }
    debugLog("CBaseUIContainer ERROR: GetBaseUIElement() \"%s\" does not exist!!!\n", name.toUtf8());
    return NULL;
}

void CBaseUIContainer::draw() {
    if(!this->bVisible) return;

    MC_UNROLL
    for(auto e : this->vElements) {
        if(e->isVisible()) {
            e->draw();
        }
    }
}

void CBaseUIContainer::draw_debug() {
    g->setColor(0xffffffff);
    g->drawLine(this->vPos.x, this->vPos.y, this->vPos.x + this->vSize.x, this->vPos.y);
    g->drawLine(this->vPos.x, this->vPos.y, this->vPos.x, this->vPos.y + this->vSize.y);
    g->drawLine(this->vPos.x, this->vPos.y + this->vSize.y, this->vPos.x + this->vSize.x, this->vPos.y + this->vSize.y);
    g->drawLine(this->vPos.x + this->vSize.x, this->vPos.y, this->vPos.x + this->vSize.x, this->vPos.y + this->vSize.y);

    g->setColor(0xff0000ff);
    for(auto &elem : this->vElements) {
        g->drawLine(elem->vPos.x, elem->vPos.y, elem->vPos.x + elem->vSize.x, elem->vPos.y);
        g->drawLine(elem->vPos.x, elem->vPos.y, elem->vPos.x, elem->vPos.y + elem->vSize.y);
        g->drawLine(elem->vPos.x, elem->vPos.y + elem->vSize.y, elem->vPos.x + elem->vSize.x,
                    elem->vPos.y + elem->vSize.y);
        g->drawLine(elem->vPos.x + elem->vSize.x, elem->vPos.y, elem->vPos.x + elem->vSize.x,
                    elem->vPos.y + elem->vSize.y);
    }
}

void CBaseUIContainer::mouse_update(bool *propagate_clicks) {
    if(!this->bVisible) return;
    CBaseUIElement::mouse_update(propagate_clicks);

    MC_UNROLL
    for(auto &elem : this->vElements) {
        elem->mouse_update(propagate_clicks);
    }
}

void CBaseUIContainer::update_pos() {
    const auto &thisPos = this->vPos;
    MC_UNROLL
    for(auto e : this->vElements) {
        e->setPos(thisPos + e->getRelPos());
    }
}

void CBaseUIContainer::onKeyUp(KeyboardEvent &e) {
    MC_UNROLL
    for(auto &elem : this->vElements) {
        if(elem->isVisible()) elem->onKeyUp(e);
    }
}
void CBaseUIContainer::onKeyDown(KeyboardEvent &e) {
    MC_UNROLL
    for(auto &elem : this->vElements) {
        if(elem->isVisible()) elem->onKeyDown(e);
    }
}

void CBaseUIContainer::onChar(KeyboardEvent &e) {
    MC_UNROLL
    for(auto &elem : this->vElements) {
        if(elem->isVisible()) elem->onChar(e);
    }
}

void CBaseUIContainer::onFocusStolen() {
    MC_UNROLL
    for(auto &elem : this->vElements) {
        elem->stealFocus();
    }
}

void CBaseUIContainer::onEnabled() {
    MC_UNROLL
    for(auto &elem : this->vElements) {
        elem->setEnabled(true);
    }
}

void CBaseUIContainer::onDisabled() {
    MC_UNROLL
    for(auto &elem : this->vElements) {
        elem->setEnabled(false);
    }
}

void CBaseUIContainer::onMouseDownOutside(bool /*left*/, bool /*right*/) { this->onFocusStolen(); }

bool CBaseUIContainer::isBusy() {
    if(!this->bVisible) return false;
    MC_UNROLL
    for(auto &elem : this->vElements) {
        if(elem->isBusy()) return true;
    }

    return false;
}

bool CBaseUIContainer::isActive() {
    if(!this->bVisible) return false;
    MC_UNROLL
    for(auto &elem : this->vElements) {
        if(elem->isActive()) return true;
    }

    return false;
}
