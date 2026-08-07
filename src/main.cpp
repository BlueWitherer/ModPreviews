#include "ImagePopup.hpp"

#include "alphalaneous.alphas_geode_utils/include/ObjectModify.hpp"
#include "alphalaneous.alphas_geode_utils/include/Utils.hpp"

#include <Geode/Geode.hpp>

#include <Geode/utils/web.hpp>

#include <Geode/ui/LazySprite.hpp>

using namespace geode::prelude;

struct RepoData {
    std::string rawURL;
    std::string repo;
};

class $nodeModify(PreviewsModPopup, ModPopup) {
    struct Fields {
        std::map<std::string, async::TaskHolder<web::WebResponse>> m_listeners;
        std::map<int, Ref<CCSprite>> m_previewSprites;
        std::map<int, Ref<CCMenuItemSpriteExtra>> m_previewButtons;

        utils::StringMap<std::string> m_repoCache;

        Ref<CCNode> m_imagesContainer;
        std::vector<Ref<LazySprite>> m_sprites;

        CCMenu* m_imagesList;
        CCMenu* m_showAllMenu;

        bool m_hasShownImages;
        bool m_isShowingDescription;
        bool m_isShowingABanner;

        std::string m_url;
    };

    static std::string baseEnumsToString(BaseType type, int size, int color) {
#define ENUMS_TO_STRING(ty_)                                             \
    case BaseType::ty_: {                                                \
        sizeStr = baseEnumToString(static_cast<ty_##BaseSize>(size));    \
        colorStr = baseEnumToString(static_cast<ty_##BaseColor>(color)); \
    } break

        const char* typeStr = baseEnumToString(type);
        const char* sizeStr;
        const char* colorStr;

        switch (type) {
            ENUMS_TO_STRING(Circle);
            ENUMS_TO_STRING(Cross);
            ENUMS_TO_STRING(Account);
            ENUMS_TO_STRING(IconSelect);
            ENUMS_TO_STRING(Leaderboard);
            ENUMS_TO_STRING(Editor);
            ENUMS_TO_STRING(Tab);
            ENUMS_TO_STRING(Category);
        };

        return fmt::format("base{}_{}_{}.png", typeStr, sizeStr, colorStr);
    };

    //as per fod's request, check everything and return to prevent side effects
    bool isSafe() {
        CCNode* githubBtn = getChildByIDRecursive("github");
        if (!githubBtn) return false;

        FLAlertLayer* self = reinterpret_cast<FLAlertLayer*>(this);
        if (!self) return false;
        if (!self->m_mainLayer) return false;

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

        bool geodeTheme = Loader::get()->getLoadedMod("geode.loader")->getSettingValue<bool>("enable-geode-theme");

        CircleBaseColor base = geodeTheme ? CircleBaseColor::DarkPurple : CircleBaseColor::Green;

        if (base == CircleBaseColor::DarkPurple && !CCSprite::createWithSpriteFrameName("geode.loader/baseCircle_Medium_DarkPurple.png")) return false;
        if (base == CircleBaseColor::Green && !CCSprite::createWithSpriteFrameName("geode.loader/baseCircle_Medium_Green.png")) return false;

        return true;
    };

    void checkIfMain(std::string url, geode::CopyableFunction<void(bool)>&& callback) {
        auto f = m_fields.self();
        auto req = web::WebRequest();

        f->m_listeners[url].spawn(
            req.get(url),
            [this, callback = std::move(callback)](web::WebResponse resp) {
                callback(resp.ok());
            });
    };

    void modify() {
        if (!isSafe()) return;

        auto githubBtn = getChildByIDRecursive("github");

        auto f = m_fields.self();

        f->m_imagesList = CCMenu::create();
        f->m_imagesList->setZOrder(1);
        f->m_imagesList->setContentSize({262, 54});
        f->m_imagesList->setAnchorPoint({0.5, 0.5});
        f->m_imagesList->ignoreAnchorPointForPosition(false);

        f->m_imagesContainer = CCNode::create();
        f->m_imagesContainer->setID("images-container"_spr);
        f->m_imagesContainer->setVisible(false);
        f->m_imagesContainer->setContentSize({267.5, 59});
        f->m_imagesContainer->setPosition({152.5, 0});
        f->m_imagesContainer->setAnchorPoint({0, 0});
        f->m_imagesContainer->addChild(f->m_imagesList);

        f->m_showAllMenu = CCMenu::create();
        f->m_showAllMenu->setContentSize({20, 54});
        f->m_showAllMenu->setPosition({f->m_imagesContainer->getContentWidth() - 2.5f, f->m_imagesContainer->getContentHeight() / 2});
        f->m_showAllMenu->ignoreAnchorPointForPosition(false);
        f->m_showAllMenu->setAnchorPoint({1, 0.5});
        f->m_showAllMenu->setZOrder(2);
        f->m_showAllMenu->setVisible(false);

        bool geodeTheme = Loader::get()->getLoadedMod("geode.loader")->getSettingValue<bool>("enable-geode-theme");

        auto moreSpr = CircleButtonSprite::createWithSpriteFrameName(
            "edit_addCBtn_001.png",
            .85f,
            (geodeTheme ? CircleBaseColor::DarkPurple : CircleBaseColor::Green));

        auto showAllBtn = CCMenuItemSpriteExtra::create(moreSpr, this, menu_selector(PreviewsModPopup::showPopup));
        showAllBtn->setPosition(f->m_showAllMenu->getContentSize() / 2);
        showAllBtn->setScale(0.4f);
        showAllBtn->setTag(1);

        showAllBtn->m_baseScale = 0.4f;

        f->m_showAllMenu->addChild(showAllBtn);
        f->m_imagesContainer->addChild(f->m_showAllMenu);

        f->m_imagesList->setPosition(f->m_imagesContainer->getContentSize() / 2);

        auto background = NineSlice::create("square02b_001.png");
        background->setContentSize(f->m_imagesContainer->getContentSize() / 0.5);
        background->setScale(0.5);
        background->setOpacity(75);
        background->setColor({0, 0, 0});
        background->setPosition(f->m_imagesContainer->getContentSize() / 2);

        f->m_imagesContainer->addChild(background);

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

                    f->m_url = fmt::format("{}/{}/previews/preview-", data.rawURL, mainBranch);

                    for (int i = 1; i <= 10; i++) {
                        std::string previewURL = fmt::format("{}{}.png", f->m_url, i);

                        auto spr = LazySprite::create({100.f, 50.f});
                        f->m_sprites.push_back(spr);

                        spr->setLoadCallback([s, i, spr](Result<> res) {
                            if (res.isOk()) s->onImageDownloadFinish(i, spr);
                        });

                        spr->loadFromUrl(previewURL);
                    };
                };
            }); });
    };

    void getRepoData(const std::string& url, geode::CopyableFunction<void(Result<RepoData>)>&& callback) {
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

        if (auto it = m_fields->m_repoCache.find(repo); it != m_fields.self()->m_repoCache.end()) rawURL = it->second;

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

                    if (auto s = self.lock()) s->m_fields->m_repoCache[repo] = raw;

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

        f->m_imagesContainer->setPositionY(30);
    };

    void resizeDescription(CCNode* description) {
        auto f = m_fields.self();

        float offset = 0;
        float gap = 0;

        if (f->m_isShowingABanner) {
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

        if (!f->m_isShowingDescription && description) {
            f->m_imagesContainer->setVisible(true);
            resizeDescription(description);
            f->m_isShowingDescription = true;
        };

        if (f->m_isShowingDescription && !description) {
            f->m_imagesContainer->setVisible(false);
            f->m_isShowingDescription = false;
        };
    };

    void listenForBanner(float dt) {
        auto f = m_fields.self();

        if (auto modtoberBanner = getChildByIDRecursive("modtober-banner")) {
            f->m_isShowingABanner = true;

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

            auto f = m_fields.self();

            firstNode->addChild(f->m_imagesContainer);

            schedule(schedule_selector(PreviewsModPopup::listenForDescription));
            schedule(schedule_selector(PreviewsModPopup::listenForBanner));
        };
    };

    void showPopup(CCObject* sender) {
        auto f = m_fields.self();

        auto popup = ImagePopup::create(sender->getTag(), f->m_previewSprites.size(), f->m_url);
        popup->show();
    };

    void onImageDownloadFinish(int id, CCSprite* spr) {
        auto f = m_fields.self();

        auto scale = 54.f / spr->getContentHeight();
        spr->setScale(scale);

        auto btn = CCMenuItemSpriteExtra::create(spr, this, menu_selector(PreviewsModPopup::showPopup));
        btn->setTag(id);
        btn->ignoreAnchorPointForPosition(true);
        btn->m_scaleMultiplier = 1.1f;

        f->m_previewSprites[id] = spr;
        f->m_previewButtons[id] = btn;

        if (!f->m_hasShownImages) {
            showImages();
            f->m_hasShownImages = true;
        };

        btn->setID(fmt::format("preview-{}", id));

        f->m_imagesList->removeAllChildren();

        CCMenuItemSpriteExtra* lastButton = nullptr;

        auto gap = 2.5f;
        auto totalWidth = 0.f;

        for (auto& [k, v] : f->m_previewButtons) {
            if (lastButton) {
                float pos = lastButton->getPositionX() + lastButton->getContentWidth();
                if (pos + v->getContentWidth() >= 262) {
                    f->m_showAllMenu->setVisible(true);
                    break;
                };

                v->setPositionX(pos + gap);
            };

            totalWidth += v->getContentWidth() + gap;
            lastButton = v;
            f->m_imagesList->addChild(v);
        };

        totalWidth -= gap;
        f->m_imagesList->setContentWidth(totalWidth);
        if (f->m_showAllMenu->isVisible()) f->m_imagesList->setPositionX((f->m_imagesContainer->getContentWidth() - f->m_showAllMenu->getContentWidth()) / 2);
    };
};
