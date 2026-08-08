#pragma once

#include <Geode/Geode.hpp>

class ImagePopup final : public geode::Popup {
protected:
    uint8_t m_page;
    uint8_t m_size;

    std::string m_url;

    geode::LazySprite* m_currentImage;
    cocos2d::CCLabelBMFont* m_imageCount;
    cocos2d::extension::CCScale9Sprite* m_bgLayer;

    std::unordered_map<uint8_t, geode::Ref<geode::LazySprite>> m_sprites;

    bool init(uint8_t page, uint8_t size, std::string url);

public:
    static ImagePopup* create(uint8_t page, uint8_t size, std::string url);

    void onPrev(CCObject* sender);
    void onNext(CCObject* sender);

    void showImage(uint8_t page);

    void onLoad(geode::LazySprite* spr);
};