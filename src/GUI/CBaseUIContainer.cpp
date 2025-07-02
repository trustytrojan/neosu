#include "CBaseUIContainer.h"

#include "Engine.h"



CBaseUIContainer::CBaseUIContainer(float Xpos, float Ypos, float Xsize, float Ysize, UString name)
    : CBaseUIElement(Xpos, Ypos, Xsize, Ysize, name) {}

CBaseUIContainer::~CBaseUIContainer() { this->clear(); }

void CBaseUIContainer::clear() {
    for(size_t i = 0; i < this->vElements.size(); i++) {
        delete this->vElements[i];
    }
    this->vElements = std::vector<CBaseUIElement *>();
}

void CBaseUIContainer::empty() { this->vElements = std::vector<CBaseUIElement *>(); }

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
            this->vElements.insert(this->vElements.begin() + std::clamp<int>(i + 1, 0, this->vElements.size()), element);
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

CBaseUIElement *CBaseUIContainer::getBaseUIElement(UString name) {
    for(size_t i = 0; i < this->vElements.size(); i++) {
        if(this->vElements[i]->getName() == name) return this->vElements[i];
    }
    debugLog("CBaseUIContainer ERROR: GetBaseUIElement() \"%s\" does not exist!!!\n", name.toUtf8());
    return NULL;
}

void CBaseUIContainer::draw() {
    if(!this->bVisible) return;

    for(size_t i = 0; i < this->vElements.size(); i++) {
        this->vElements[i]->draw();
    }
}

void CBaseUIContainer::draw_debug() {
    g->setColor(0xffffffff);
    g->drawLine(this->vPos.x, this->vPos.y, this->vPos.x + this->vSize.x, this->vPos.y);
    g->drawLine(this->vPos.x, this->vPos.y, this->vPos.x, this->vPos.y + this->vSize.y);
    g->drawLine(this->vPos.x, this->vPos.y + this->vSize.y, this->vPos.x + this->vSize.x, this->vPos.y + this->vSize.y);
    g->drawLine(this->vPos.x + this->vSize.x, this->vPos.y, this->vPos.x + this->vSize.x, this->vPos.y + this->vSize.y);

    g->setColor(0xff0000ff);
    for(size_t i = 0; i < this->vElements.size(); i++) {
        g->drawLine(this->vElements[i]->vPos.x, this->vElements[i]->vPos.y,
                    this->vElements[i]->vPos.x + this->vElements[i]->vSize.x, this->vElements[i]->vPos.y);
        g->drawLine(this->vElements[i]->vPos.x, this->vElements[i]->vPos.y, this->vElements[i]->vPos.x,
                    this->vElements[i]->vPos.y + this->vElements[i]->vSize.y);
        g->drawLine(this->vElements[i]->vPos.x, this->vElements[i]->vPos.y + this->vElements[i]->vSize.y,
                    this->vElements[i]->vPos.x + this->vElements[i]->vSize.x,
                    this->vElements[i]->vPos.y + this->vElements[i]->vSize.y);
        g->drawLine(this->vElements[i]->vPos.x + this->vElements[i]->vSize.x, this->vElements[i]->vPos.y,
                    this->vElements[i]->vPos.x + this->vElements[i]->vSize.x,
                    this->vElements[i]->vPos.y + this->vElements[i]->vSize.y);
    }
}

void CBaseUIContainer::mouse_update(bool *propagate_clicks) {
    if(!this->bVisible) return;
    CBaseUIElement::mouse_update(propagate_clicks);

    for(size_t i = 0; i < this->vElements.size(); i++) {
        this->vElements[i]->mouse_update(propagate_clicks);
    }
}

void CBaseUIContainer::update_pos() {
    for(size_t i = 0; i < this->vElements.size(); i++) {
        this->vElements[i]->setPos(this->vPos + this->vElements[i]->getRelPos());
    }
}

void CBaseUIContainer::onKeyUp(KeyboardEvent &e) {
    for(size_t i = 0; i < this->vElements.size(); i++) {
        if(this->vElements[i]->isVisible()) this->vElements[i]->onKeyUp(e);
    }
}
void CBaseUIContainer::onKeyDown(KeyboardEvent &e) {
    for(size_t i = 0; i < this->vElements.size(); i++) {
        if(this->vElements[i]->isVisible()) this->vElements[i]->onKeyDown(e);
    }
}

void CBaseUIContainer::onChar(KeyboardEvent &e) {
    for(size_t i = 0; i < this->vElements.size(); i++) {
        if(this->vElements[i]->isVisible()) this->vElements[i]->onChar(e);
    }
}

void CBaseUIContainer::onFocusStolen() {
    for(size_t i = 0; i < this->vElements.size(); i++) {
        this->vElements[i]->stealFocus();
    }
}

void CBaseUIContainer::onEnabled() {
    for(size_t i = 0; i < this->vElements.size(); i++) {
        this->vElements[i]->setEnabled(true);
    }
}

void CBaseUIContainer::onDisabled() {
    for(size_t i = 0; i < this->vElements.size(); i++) {
        this->vElements[i]->setEnabled(false);
    }
}

void CBaseUIContainer::onMouseDownOutside() { this->onFocusStolen(); }

bool CBaseUIContainer::isBusy() {
    if(!this->bVisible) return false;

    for(size_t i = 0; i < this->vElements.size(); i++) {
        if(this->vElements[i]->isBusy()) return true;
    }

    return false;
}

bool CBaseUIContainer::isActive() {
    if(!this->bVisible) return false;

    for(size_t i = 0; i < this->vElements.size(); i++) {
        if(this->vElements[i]->isActive()) return true;
    }

    return false;
}
