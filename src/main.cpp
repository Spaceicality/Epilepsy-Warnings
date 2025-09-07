#include <Geode/Geode.hpp>
#include <Geode/modify/LevelInfoLayer.hpp>
#include <Geode/utils/web.hpp>

using namespace geode::prelude;

class $modify(MyLevelInfoLayer, LevelInfoLayer) {
    struct Fields {
        EventListener<web::WebTask> m_listener;
        bool m_shouldWarn = false;
    };

public:
    bool init(GJGameLevel* level, bool p1) {
        if (!LevelInfoLayer::init(level, p1))
            return false;

        int levelID = level->m_levelID.value();

        if (levelID < 1) {
            return true;
        }

        auto url = fmt::format(
            "https://epilepsywarningapi.fluxitegmd.workers.dev/id/{}", 
            levelID
        );

        m_fields->m_listener.bind([this, levelID](web::WebTask::Event* event) {
            if (auto* response = event->getValue()) {
                auto result = response->string().unwrapOr("false");
                if (result == "true") {
                    m_fields->m_shouldWarn = true;
                }
            }
        });

        auto request = web::WebRequest();
        m_fields->m_listener.setFilter(request.get(url));

        return true;
    }

    void onEnterTransitionDidFinish() {
        LevelInfoLayer::onEnterTransitionDidFinish();

        if (m_fields->m_shouldWarn) {
            auto warningAlert = FLAlertLayer::create(
                "WARNING!",
                "This level is in the Epileptic Warnings Database and may contain seizure-inducing effects!",
                "OK"
            );

            if (warningAlert) {
                FMODAudioEngine::sharedEngine()->playEffect("chestClick.ogg");
                CCDirector::sharedDirector()->getRunningScene()->addChild(warningAlert, 1000);
            }

            m_fields->m_shouldWarn = false;
        }
    }
};




