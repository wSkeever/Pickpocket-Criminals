#include "RE/Skyrim.h"
#include "SKSE/SKSE.h"
using namespace RE;

namespace PickpocketCriminals {

    using IsStealingContainerSignature = bool(Actor*, TESForm*, int);
    REL::Relocation<IsStealingContainerSignature> is_stealing_container_original;
    using PickpocketSignature = ObjectRefHandle*(
        TESObjectREFR*,
        ObjectRefHandle*,
        TESBoundObject*,
        std::int32_t,
        ITEM_REMOVE_REASON,
        ExtraDataList*,
        TESObjectREFR*,
        const NiPoint3*,
        const NiPoint3*
    );
    REL::Relocation<PickpocketSignature> pickpocket_original;

    bool IsPickpocketingCriminal(TESForm* a_taker, TESForm* a_owner, int a_value) {
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
        if (IsPickpocketingCriminal(a_taker, a_owner, a_value)) {
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

    ObjectRefHandle* PickpocketReplacement(
        TESObjectREFR* a_this,
        ObjectRefHandle* a_out,
        TESBoundObject* a_item,
        std::int32_t a_count,
        ITEM_REMOVE_REASON a_reason,
        ExtraDataList* a_extraList,
        TESObjectREFR* a_moveToRef,
        const NiPoint3* a_dropLoc,
        const NiPoint3* a_rotate
    ) {
        if (IsPickpocketingCriminal(a_moveToRef, a_this, a_count) && a_reason == ITEM_REMOVE_REASON::kSteal) {
            a_reason = ITEM_REMOVE_REASON::kRemove;
        }
        *a_out = a_this->RemoveItem(a_item, a_count, a_reason, a_extraList, a_moveToRef, a_dropLoc, a_rotate);
        return a_out;
    }

    void InstallPickpocketHook() {
        REL::RelocationID hook{50212, 51141, 50212};
        ptrdiff_t offset = REL::VariantOffset(0x2EE, 0x2D5, 0x2EE).offset();
        auto& trampoline = SKSE::GetTrampoline();
        pickpocket_original = trampoline.write_call<6>(hook.address() + offset, PickpocketReplacement);
    }

    SKSEPluginLoad(const SKSE::LoadInterface* skse) {
        SKSE::Init(skse);
        SKSE::AllocTrampoline(14 * 2);
        SKSE::GetMessagingInterface()->RegisterListener([](SKSE::MessagingInterface::Message* message) {
            if (message->type == SKSE::MessagingInterface::kDataLoaded) {
                InstallIsStealingContainerHook();
                InstallPickpocketHook();
            }
        });

        return true;
    }
}