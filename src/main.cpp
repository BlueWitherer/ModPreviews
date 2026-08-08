// original mod by alphalaneous
// source inherited & now maintained by me !!!
#include "ImagePopup.h"

#include "Macros.h"

#include "alphalaneous.alphas_geode_utils/include/Utils.hpp"
#include "alphalaneous.alphas_geode_utils/include/ObjectModify.hpp"

#include <Geode/Geode.hpp>

#include <Geode/utils/web.hpp>

using namespace geode::prelude;

struct RepoData final {
    std::string rawURL;
    std::string repo;
};

class $nodeModify(PreviewsModPopup, ModPopup) {
    struct Fields final {
        utils::StringMap<async::TaskHolder<web::WebResponse>> listeners;

        std::unordered_map<uint8_t, Ref<CCSprite>> previewSprites;
        std::map<uint8_t, Ref<CCMenuItemSpriteExtra>> previewButtons;

        utils::StringMap<std::string> repoCache;

        Ref<CCNode> imagesContainer;
        std::vector<Ref<LazySprite>> sprites;

        CCMenu* imagesList;
        CCMenu* showAllMenu;

        bool hasShownImages;
        bool isShowingDescription;
        bool isShowingABanner;

        std::string url;
    };

    // as per fod's request, check everything and return to prevent side effects
    bool isSafe() {
        if (auto githubBtn = getChildByIDRecursive("github")) {
            if (auto self = reinterpret_cast<FLAlertLayer*>(this)) {
                std::optional<CCNode*> firstNodeOpt = alpha::utils::cocos::getChildByClassName(self->m_mainLayer, "cocos2d::CCNode", 0);
                if (!firstNodeOpt.has_value()) return false;

                if (CCString* url = static_cast<CCString*>(githubBtn->getUserObject("url"))) {
                    std::string githubURL = url->getCString();
                    if (githubURL.empty()) return false;
                } else {
                    return false;
                };

                //check if sprites exist ig
                if (!CCSprite::createWithSpriteFrameName("edit_addCBtn_001.png")) return false;

                IS_GEODE_THEME(auto geodeTheme);

                auto base = geodeTheme ? CircleBaseColor::DarkPurple : CircleBaseColor::Green;

                if (base == CircleBaseColor::DarkPurple && !CCSprite::createWithSpriteFrameName("geode.loader/baseCircle_Medium_DarkPurple.png")) return false;
                if (base == CircleBaseColor::Green && !CCSprite::createWithSpriteFrameName("geode.loader/baseCircle_Medium_Green.png")) return false;

                return true;
            };
        };

        return false;
    };

    void checkIfMain(std::string url, CopyableFunction<void(bool)>&& callback) {
        auto f = m_fields.self();
        auto req = web::WebRequest();

        f->listeners[url].spawn(
            req.get(url),
            [this, callback = std::move(callback)](web::WebResponse resp) {
                callback(resp.ok());
            });
    };

    void modify() {
        if (!isSafe()) return;

        auto githubBtn = getChildByIDRecursive("github");

        auto f = m_fields.self();

        f->imagesList = CCMenu::create();
        f->imagesList->setZOrder(1);
        f->imagesList->setContentSize({262, 54});
        f->imagesList->setAnchorPoint({0.5, 0.5});
        f->imagesList->ignoreAnchorPointForPosition(false);

        f->imagesContainer = CCNode::create();
        f->imagesContainer->setID("images-container"_spr);
        f->imagesContainer->setVisible(false);
        f->imagesContainer->setContentSize({267.5, 59});
        f->imagesContainer->setPosition({152.5, 0});
        f->imagesContainer->setAnchorPoint({0, 0});
        f->imagesContainer->addChild(f->imagesList);

        f->showAllMenu = CCMenu::create();
        f->showAllMenu->setContentSize({20, 54});
        f->showAllMenu->setPosition({f->imagesContainer->getContentWidth() - 2.5f, f->imagesContainer->getContentHeight() / 2});
        f->showAllMenu->ignoreAnchorPointForPosition(false);
        f->showAllMenu->setAnchorPoint({1, 0.5});
        f->showAllMenu->setZOrder(2);
        f->showAllMenu->setVisible(false);

        IS_GEODE_THEME(auto geodeTheme);

        auto moreSpr = CircleButtonSprite::createWithSpriteFrameName(
            "edit_addCBtn_001.png",
            .85f,
            (geodeTheme ? CircleBaseColor::DarkPurple : CircleBaseColor::Green));

        auto showAllBtn = CCMenuItemSpriteExtra::create(moreSpr, this, menu_selector(PreviewsModPopup::showPopup));
        showAllBtn->setPosition(f->showAllMenu->getContentSize() / 2);
        showAllBtn->setScale(0.4f);
        showAllBtn->setTag(1);

        showAllBtn->m_baseScale = 0.4f;

        f->showAllMenu->addChild(showAllBtn);
        f->imagesContainer->addChild(f->showAllMenu);

        f->imagesList->setPosition(f->imagesContainer->getContentSize() / 2);

        auto background = NineSlice::create("square02b_001.png");
        background->setContentSize(f->imagesContainer->getContentSize() / 0.5);
        background->setScale(0.5);
        background->setOpacity(75);
        background->setColor({0, 0, 0});
        background->setPosition(f->imagesContainer->getContentSize() / 2);

        f->imagesContainer->addChild(background);

        auto url = static_cast<CCString*>(githubBtn->getUserObject("url"));

        std::string sourceURL = url->getCString();
        if (sourceURL.empty()) return;

        utils::string::toLowerIP(sourceURL);

        getRepoData(sourceURL, [this, sourceURL](Result<RepoData> dataRes) {
            if (dataRes.isErr()) {
                log::error("{}", dataRes.unwrapErr());
                return;
            };

            log::debug("Retrieved repository data for {}", sourceURL);

            auto data = std::move(dataRes).unwrap();

            auto modJsonUrl = fmt::format("{}/main/mod.json", data.rawURL);

            checkIfMain(modJsonUrl, [self = WeakRef(this), data](bool main) {
                if (auto s = self.lock()) {
                    std::string mainBranch = "main";
                    if (!main) mainBranch = "master";

                    auto f = s->m_fields.self();

                    f->url = fmt::format("{}/{}/previews/preview-", data.rawURL, mainBranch);

                    for (uint8_t i = 1; i <= 10; i++) {
                        std::string previewURL = fmt::format("{}{}.png", f->url, i);

                        auto spr = LazySprite::create({100.f, 50.f});
                        f->sprites.push_back(spr);

                        spr->setLoadCallback([s, i, spr](Result<> res) {
                            if (res.isOk()) s->onImageDownloadFinish(i, spr);
                        });

                        spr->loadFromUrl(previewURL);
                    };
                };
            }); });
    };

    void getRepoData(const std::string& url, CopyableFunction<void(Result<RepoData>)>&& callback) {
        auto split = utils::string::split(url, "://");
        if (split.size() < 2) {
            callback(Err("Invalid HTTPS URL"));
            return;
        };

        auto& postURL = split[1];
        std::string prefixWWW = "www.";

        if (postURL.starts_with(prefixWWW)) postURL.erase(0, prefixWWW.size());

        auto splitSlash = utils::string::split(postURL, "/");
        if (splitSlash.size() < 2) {
            callback(Err("Invalid URL"));
            return;
        };

        auto& platformURL = splitSlash[0];
        auto repo = postURL.substr(platformURL.size() + 1);

        std::string rawURL = "";

        if (platformURL == "github.com") rawURL = fmt::format("https://raw.githubusercontent.com/{}", repo);
        if (platformURL == "gitlab.com") rawURL = fmt::format("https://gitlab.com/{}/-/raw", repo);

        if (auto it = m_fields->repoCache.find(repo); it != m_fields->repoCache.end()) rawURL = it->second;

        if (!rawURL.empty()) return callback(
            Ok(
                RepoData{
                    .rawURL = rawURL,
                    .repo = repo,
                }));

        auto req = web::WebRequest();

        auto fullBaseURL = fmt::format("https://{}/api/v1/version", platformURL);
        log::trace("Sending request to {} to check Git instance version", fullBaseURL);

        async::spawn(
            req.get(fullBaseURL),
            [self = WeakRef(this), platformURL, repo, fullBaseURL, callback = std::move(callback)](web::WebResponse res) {
                if (!res.ok()) return callback(Err(fmt::format("Failed to get response from {}: {}", fullBaseURL, res.errorMessage())));

                auto jsonRes = res.json();
                if (jsonRes.isErr()) return callback(Err(fmt::format("Failed to parse JSON from {}: {}", fullBaseURL, jsonRes.unwrapErr())));

                auto json = std::move(jsonRes).unwrap();

                auto versionRes = json["version"].asString();
                if (versionRes.isErr()) return callback(Err(fmt::format("Failed to get version from {}: {}", fullBaseURL, jsonRes.unwrapErr())));

                auto version = std::move(versionRes).unwrap();
                log::trace("Git instance version: {}", version);

                if (utils::string::contains(version, "+gitea-")) {
                    auto raw = fmt::format("https://{}/{}/raw/branch", platformURL, repo);
                    log::info("Detected Forgejo instance at {}, using {}", platformURL, raw);

                    if (auto s = self.lock()) s->m_fields->repoCache[repo] = raw;

                    return callback(
                        Ok(
                            RepoData{
                                .rawURL = raw,
                                .repo = repo,
                            }));
                };

                callback(Err(fmt::format("Unsupported platform for URL: {}", fullBaseURL)));
            });
    };

    void resizeForBanner(CCNode* banner) {
        auto f = m_fields.self();

        auto description = getChildByIDRecursive("description-container");
        resizeDescription(description);

        f->imagesContainer->setPositionY(30);
    };

    void resizeDescription(CCNode* description) {
        auto f = m_fields.self();

        float offset = 0;
        float gap = 0;

        if (f->isShowingABanner) {
            gap = 10;
            offset = 25 + gap;
        };

        description->setContentHeight(192 - offset);
        description->setPositionY(67 + offset - gap / 2);

        if (auto textArea = static_cast<MDTextArea*>(description->getChildByID("textarea"))) {
            textArea->setContentHeight(description->getContentHeight());
            textArea->setAnchorPoint({0.5, 0});
            textArea->setPositionY(0);

            if (auto background = textArea->getChildByType<NineSlice*>(0)) {
                background->setContentHeight(description->getContentHeight() / background->getScale());
                background->setAnchorPoint({0.5, 0});
                background->setPositionY(0);
            };

            if (auto scrollLayer = textArea->getChildByType<ScrollLayer*>(0)) {
                scrollLayer->setContentHeight(description->getContentHeight());
                scrollLayer->scrollToTop();
            };
        };
    };

    void listenForDescription(float dt) {
        auto f = m_fields.self();

        auto description = getChildByIDRecursive("description-container");

        if (!f->isShowingDescription && description) {
            f->imagesContainer->setVisible(true);
            resizeDescription(description);

            f->isShowingDescription = true;
        };

        if (f->isShowingDescription && !description) {
            f->imagesContainer->setVisible(false);

            f->isShowingDescription = false;
        };
    };

    void listenForBanner(float dt) {
        auto f = m_fields.self();

        if (auto modtoberBanner = getChildByIDRecursive("modtober-banner")) {
            f->isShowingABanner = true;

            resizeForBanner(modtoberBanner);
            unschedule(schedule_selector(PreviewsModPopup::listenForBanner));
        };
    };

    CCSprite* createSprite(CCImage* img) {
        auto texture = new CCTexture2D();
        CCSprite* spr = nullptr;

        if (texture->initWithImage(img)) spr = CCSprite::createWithTexture(texture);
        texture->release();

        return spr;
    };

    void showImages() {
        if (auto self = reinterpret_cast<FLAlertLayer*>(this)) {
            auto firstNodeOpt = alpha::utils::cocos::getChildByClassName(self->m_mainLayer, "cocos2d::CCNode", 0);
            if (!firstNodeOpt.has_value()) return;

            auto firstNode = firstNodeOpt.value();

            firstNode->addChild(m_fields->imagesContainer);

            schedule(schedule_selector(PreviewsModPopup::listenForDescription));
            schedule(schedule_selector(PreviewsModPopup::listenForBanner));
        };
    };

    void showPopup(CCObject* sender) {
        auto f = m_fields.self();

        auto popup = ImagePopup::create(sender->getTag(), f->previewSprites.size(), f->url);
        popup->show();
    };

    void onImageDownloadFinish(uint8_t id, CCSprite* spr) {
        auto f = m_fields.self();

        auto scale = 54.f / spr->getContentHeight();
        spr->setScale(scale);

        auto btn = CCMenuItemSpriteExtra::create(spr, this, menu_selector(PreviewsModPopup::showPopup));
        btn->setTag(id);
        btn->ignoreAnchorPointForPosition(true);

        btn->m_scaleMultiplier = 1.1f;

        f->previewSprites[id] = spr;
        f->previewButtons[id] = btn;

        if (!f->hasShownImages) {
            showImages();
            f->hasShownImages = true;
        };

        btn->setID(fmt::format("preview-{}", id));

        f->imagesList->removeAllChildren();

        CCMenuItemSpriteExtra* lastButton = nullptr;

        auto gap = 2.5f;
        auto totalWidth = 0.f;

        for (auto& [k, v] : f->previewButtons) {
            if (lastButton) {
                auto pos = lastButton->getPositionX() + lastButton->getContentWidth();

                if (pos + v->getContentWidth() >= 262.f) {
                    f->showAllMenu->setVisible(true);
                    break;
                };

                v->setPositionX(pos + gap);
            };

            totalWidth += v->getContentWidth() + gap;
            lastButton = v;

            f->imagesList->addChild(v);
        };

        totalWidth -= gap;

        f->imagesList->setContentWidth(totalWidth);

        if (f->showAllMenu->isVisible()) f->imagesList->setPositionX((f->imagesContainer->getContentWidth() - f->showAllMenu->getContentWidth()) / 2);
    };
};
