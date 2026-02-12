#include "RE/Skyrim.h"
#include "SKSE/SKSE.h"

namespace PickpocketCriminals {

    struct MenuOpenCloseSink;

    RE::TESObjectREFR* menuTarget{nullptr};

    inline RE::ExtraDataList* CreateExtraDataList() {
        static bool is_se = !REL::Module::IsAE();
        auto a_this = (RE::ExtraDataList*)RE::malloc(is_se ? 0x18 : 0x20);
        using func_t = RE::ExtraDataList*(RE::ExtraDataList*);
        REL::Relocation<func_t> func{REL::RelocationID{11437, 11583}};
        return func(a_this);
    }

    struct ContainerChangedSink : public RE::BSTEventSink<RE::TESContainerChangedEvent> {
        RE::BSEventNotifyControl ProcessEvent(const RE::TESContainerChangedEvent* a_event,
                                              RE::BSTEventSource<RE::TESContainerChangedEvent>*) override {
            if (!a_event) {
                return RE::BSEventNotifyControl::kContinue;
            }

            const auto fromRefID = a_event->oldContainer;
            if (!fromRefID) {
                return RE::BSEventNotifyControl::kContinue;
            }

            const auto fromRef = RE::TESObjectREFR::LookupByID(fromRefID);
            if (!fromRef) {
                return RE::BSEventNotifyControl::kContinue;
            }

            if (fromRef != menuTarget) {
                return RE::BSEventNotifyControl::kContinue;
            }
            
            const auto toRefID = a_event->newContainer;
            if (!toRefID) {
                return RE::BSEventNotifyControl::kContinue;
            }

            const auto toRef = RE::TESObjectREFR::LookupByID(toRefID);
            if (!toRef) {
                return RE::BSEventNotifyControl::kContinue;
            }

            const auto player = RE::PlayerCharacter::GetSingleton();
            if (!player) {
                return RE::BSEventNotifyControl::kContinue;
            }

            if (toRef != player) {
                return RE::BSEventNotifyControl::kContinue;
            }

            const auto playerBase = player->GetActorBase();
            if (!playerBase) {
                return RE::BSEventNotifyControl::kContinue;
            }

            const auto fromActor = fromRef->As<RE::Actor>();
            if (!fromActor) {
                return RE::BSEventNotifyControl::kContinue;
            }

            const auto fromActorBase = fromActor->GetActorBase();
            const auto fromActorTemplate = fromActor->GetTemplateBase();
            if (!fromActorTemplate && !fromActorBase) {
                return RE::BSEventNotifyControl::kContinue;
            }

            const auto itemID = a_event->baseObj;
            if (!itemID) {
                return RE::BSEventNotifyControl::kContinue;
            }

            const auto itemBase = RE::TESForm::LookupByID(itemID);
            if (!itemBase) {
                return RE::BSEventNotifyControl::kContinue;
            }

            const auto itemBaseBount = itemBase->As<RE::TESBoundObject>();                

            const auto inventoryChanges = player->GetInventoryChanges();
            if (!inventoryChanges) {
                return RE::BSEventNotifyControl::kContinue;
            }
            auto* entryList = inventoryChanges->entryList;
            if (!entryList) {
                return RE::BSEventNotifyControl::kContinue;
            }
            for (auto* entry : *entryList) {
                if (!entry) {
                    continue;
                }
                if (entry->object != itemBaseBount) {
                    continue;
                }

                const auto entryOwner = entry->GetOwner();
                if (!entryOwner) {
                    continue;
                }
                const auto entryOwnerBase = entryOwner->As<RE::TESNPC>();
                if (!entryOwnerBase) {
                    continue;
                }
                if (entryOwnerBase->crimeFaction) {
                    continue;
                }

                for (auto* extraList : *entry->extraLists) {
                    if (!extraList) {
                        continue;
                    }
                    extraList->SetOwner(NULL);
                }
            }
            return RE::BSEventNotifyControl::kContinue;
        }
    };

    static ContainerChangedSink g_containerChangedSink;

    struct MenuOpenCloseSink : public RE::BSTEventSink<RE::MenuOpenCloseEvent> {
        RE::BSEventNotifyControl ProcessEvent(const RE::MenuOpenCloseEvent* a_event,
                                              RE::BSTEventSource<RE::MenuOpenCloseEvent>*) override {
            if (!a_event) {
                return RE::BSEventNotifyControl::kContinue;
            }

            if (a_event->menuName != RE::BSFixedString("ContainerMenu")) {
                return RE::BSEventNotifyControl::kContinue;
            }

            if (!a_event->opening) {
                menuTarget = nullptr;
                return RE::BSEventNotifyControl::kContinue;
            }

            const auto ui = RE::UI::GetSingleton();
            if (!ui) {
                return RE::BSEventNotifyControl::kContinue;
            }

            const auto containerMenu = ui->GetMenu<RE::ContainerMenu>();
            if (!containerMenu) {
                return RE::BSEventNotifyControl::kContinue;
            }

            if (containerMenu->GetContainerMode() != RE::ContainerMenu::ContainerMode::kPickpocket) {
                return RE::BSEventNotifyControl::kContinue;
            }

            const auto targetHandle = RE::ContainerMenu::GetTargetRefHandle();
            const auto targetRef = RE::TESObjectREFR::LookupByHandle(targetHandle);
            if (!targetRef) {
                return RE::BSEventNotifyControl::kContinue;
            }

            const auto actor = targetRef->As<RE::Actor>();
            if (!actor) {
                return RE::BSEventNotifyControl::kContinue;
            }

            if (actor->GetCrimeFaction()) {
                return RE::BSEventNotifyControl::kContinue;
            }

            RE::ConsoleLog::GetSingleton()->Print(fmt::format("Opened pickpocket menu for: {} {:08X}.", actor->GetDisplayFullName(), actor->GetFormID()).c_str());

            menuTarget = actor;
            return RE::BSEventNotifyControl::kContinue;
        }
    };

    static MenuOpenCloseSink g_menuOpenCloseSink;

    SKSEPluginLoad(const SKSE::LoadInterface* skse) {
        SKSE::Init(skse);

        SKSE::GetMessagingInterface()->RegisterListener([](SKSE::MessagingInterface::Message* message) {
            if (message->type == SKSE::MessagingInterface::kDataLoaded) {
                if (auto ui = RE::UI::GetSingleton()) {
                    ui->AddEventSink(&g_menuOpenCloseSink);
                }
                if (auto holder = RE::ScriptEventSourceHolder::GetSingleton()) {
                    holder->AddEventSink(&g_containerChangedSink);
                }
            }
        });

        return true;
    }

}