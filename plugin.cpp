SKSEPluginLoad(const SKSE::LoadInterface *skse) {
    SKSE::Init(skse);

    struct MenuOpenCloseSink : public RE::BSTEventSink<RE::MenuOpenCloseEvent> {
        RE::BSEventNotifyControl ProcessEvent(const RE::MenuOpenCloseEvent* a_event,
                                              RE::BSTEventSource<RE::MenuOpenCloseEvent>*) override {
            if (!a_event) {
                return RE::BSEventNotifyControl::kContinue;
            }

            if (!a_event->opening) {
                return RE::BSEventNotifyControl::kContinue;
            }

            if (a_event->menuName != RE::BSFixedString("ContainerMenu")) {
                return RE::BSEventNotifyControl::kContinue;
            }

            auto ui = RE::UI::GetSingleton();
            if (!ui) {
                return RE::BSEventNotifyControl::kContinue;
            }

            auto containerMenu = ui->GetMenu<RE::ContainerMenu>();
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

            auto actor = targetRef->As<RE::Actor>();
            if (!actor) {
                return RE::BSEventNotifyControl::kContinue;
            }

            if (actor->GetCrimeFaction()) {
                return RE::BSEventNotifyControl::kContinue;
            }

            auto player = RE::PlayerCharacter::GetSingleton();
            if (player) {
                if (!actor->IsHostileToActor(player)) {
                    return RE::BSEventNotifyControl::kContinue;
                }
            }

            const auto inventory = actor->GetInventory();
            for (auto& kv : inventory) {
                auto& entryPair = kv.second;
                RE::InventoryEntryData* entry = entryPair.second ? entryPair.second.get() : nullptr;
                if (!entry) {
                    continue;
                }

                RE::TESForm* entryOwner = entry->GetOwner();
                RE::TESForm* actorBase = actor->GetBaseObject();
                if (entryOwner && entryOwner != actorBase) {
                    continue;
                }

                if (!entry->extraLists) {
                    continue;
                }

                for (auto extraIt = entry->extraLists->begin(); extraIt != entry->extraLists->end(); ++extraIt) {
                    RE::ExtraDataList* extra = *extraIt;
                    if (!extra) {
                        continue;
                    }
                    if (player) {
                        extra->SetOwner(player->GetBaseObject());
                    }
                }
            }

            return RE::BSEventNotifyControl::kContinue;
        }
    };

    static MenuOpenCloseSink g_menuOpenCloseSink;

    SKSE::GetMessagingInterface()->RegisterListener([](SKSE::MessagingInterface::Message *message) {
        if (message->type == SKSE::MessagingInterface::kDataLoaded) {
            if (auto ui = RE::UI::GetSingleton()) {
                ui->AddEventSink(&g_menuOpenCloseSink);
            }
        }
    });

    return true;
}

