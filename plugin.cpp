#include "RE/Skyrim.h"
#include "SKSE/SKSE.h"
using namespace RE;

namespace PickpocketCriminals {

    using IsStealingContainerSignature = bool(Actor*, TESForm*, int);
    REL::Relocation<IsStealingContainerSignature> is_stealing_container_original;

    bool IsPickpocketingCriminal(TESForm* a_taker, TESForm* a_owner) {
        if (!a_taker || !a_owner) {
            return false;
        }
        const auto ui = UI::GetSingleton();
        if (!ui) {
            return false;
        }
        if (ContainerMenu::GetContainerMode() != ContainerMenu::ContainerMode::kPickpocket) {
            return false;
        }
        if (!a_taker->IsPlayerRef()) {
            return false;
        }
        auto* ownerActor = a_owner->As<Actor>();
        if (!ownerActor || ownerActor->GetCrimeFaction()) {
            return false;
        }
        return true;
    }

    bool IsStealingContainerReplacement(Actor* a_taker, TESForm* a_owner, int a_value) {
        if (IsPickpocketingCriminal(a_taker, a_owner)) {
            return true;
        }
        return is_stealing_container_original(a_taker, a_owner, a_value);
    }

    void InstallIsStealingContainerHook() {
        REL::RelocationID hook{50231, 51160, 50231};
        ptrdiff_t offset = REL::VariantOffset(0x14D, 0x135, 0x14D).offset();
        auto& trampoline = SKSE::GetTrampoline();
        is_stealing_container_original = trampoline.write_call<5>(hook.address() + offset, IsStealingContainerReplacement);
    }

    void ClearOwnership(TESForm* newContainer, TESForm* oldContainer, TESForm* itemBase) {
        if (!newContainer || !oldContainer || !itemBase) {
            return;
        }
        const auto itemBaseBound = itemBase->As<TESBoundObject>();
        if (!itemBaseBound) {
            return;
        }
        const auto playerRef = newContainer->As<Actor>();
        if (!playerRef || !playerRef->IsPlayerRef()) {
            return;
        }
        const auto inventoryChanges = playerRef->GetInventoryChanges();
        if (!inventoryChanges) {
            return;
        }
        auto* entryList = inventoryChanges->entryList;
        if (!entryList) {
            return;
        }
        for (auto* entry : *entryList) {
            if (!entry || entry->object != itemBaseBound) {
                continue;
            }
            const auto entryOwner = entry->GetOwner();
            if (!entryOwner) {
                continue;
            }
            const auto entryOwnerBase = entryOwner->As<TESNPC>();
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
    }

    struct ContainerChangedSink : public BSTEventSink<TESContainerChangedEvent> {
        BSEventNotifyControl ProcessEvent(const TESContainerChangedEvent* a_event,
                                              BSTEventSource<TESContainerChangedEvent>*) override {
            if (!a_event) {
                return BSEventNotifyControl::kContinue;
            }
            const auto fromRef = TESObjectREFR::LookupByID(a_event->oldContainer);
            const auto toRef = TESObjectREFR::LookupByID(a_event->newContainer);
            if (IsPickpocketingCriminal(fromRef, toRef)) {
                const auto itemBase = TESObjectREFR::LookupByID(a_event->baseObj);
                ClearOwnership(toRef, fromRef, itemBase);
            }
            return BSEventNotifyControl::kContinue;
        }
    };

    static ContainerChangedSink g_containerChangedSink;

    void InstallContainerChangedSink() {
        if (auto holder = ScriptEventSourceHolder::GetSingleton()) {
            holder->AddEventSink(&g_containerChangedSink);
        }
    }

    SKSEPluginLoad(const SKSE::LoadInterface* skse) {
        SKSE::Init(skse);
        SKSE::AllocTrampoline(14);
        SKSE::GetMessagingInterface()->RegisterListener([](SKSE::MessagingInterface::Message* message) {
            if (message->type == SKSE::MessagingInterface::kDataLoaded) {
                InstallIsStealingContainerHook();
                InstallContainerChangedSink();
            }
        });

        return true;
    }
}