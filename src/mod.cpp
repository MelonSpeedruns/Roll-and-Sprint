#include "mods/hook.hpp"
#include "mods/service.hpp"
#include "mods/svc/hook.h"

#include "d/actor/d_a_alink.h"
#include "d/actor/d_a_horse.h"
#include "d/actor/d_a_midna.h"
#include "d/d_com_inf_game.h"

DEFINE_MOD();
IMPORT_SERVICE(HookService, svc_hook);

DEFINE_HOOK(&daAlink_c::procMoveInit, LinkProcMoveInit);
DEFINE_HOOK(&daAlink_c::setDoubleAnime, LinkSetDoubleAnime);
DEFINE_HOOK(&daAlink_c::checkNormalAction, LinkCheckCutAction);

bool running = false;

extern "C" {

HookAction link_proc_move_init_pre(ModContext* ctx, void* args, void* retval, void*) {
    daAlink_c* link = daAlink_getAlinkActorClass();
    if (!running && link && link->mProcID == daAlink_c::daAlink_PROC::PROC_FRONT_ROLL) {
        if (mDoCPd_c::getHoldA(0) != 0 && !link->checkEventRun())
        {
            if (link->mEquipItem != 0xFF) {
                link->allUnequip(0);
            }

            link->setSwordVoiceSe(Z2SE_AL_V_THROW_IB);
            running = true;
        }
    }
    return HOOK_CONTINUE;
}

HookAction link_set_double_anime_pre(ModContext* ctx, void* args, void*, void*) {
    daAlink_c* link = daAlink_getAlinkActorClass();
    if (link && running) {
        daAlink_c::daAlink_ANM& linkAnm = mods::arg_ref<daAlink_c::daAlink_ANM>(args, 5);

        if (linkAnm == link->ANM_RUN) {
            f32& anmSpeed = mods::arg_ref<f32>(args, 3);
            anmSpeed = 2.0f;
            linkAnm = link->ANM_RUN_B;
            link->mNormalSpeed = 40.0f;
        }
    }
    return HOOK_CONTINUE;
}

HookAction link_check_cut_action_pre(ModContext* ctx, void* args, void* retval, void*) {
    daAlink_c* link = daAlink_getAlinkActorClass();
    if (link && running && link->swordSwingTrigger()) {
        if (!link->checkSwordEquipAnime()) {
            if (link->checkWoodSwordEquip()) {
                link->seStartSwordCut(Z2SE_AL_ITEM_TAKEOUT_FAST);
            } else {
                link->seStartSwordCut(Z2SE_AL_SWORD_PULLOUT);
            }
        }

        link->mEquipItem = 0x103;
        link->setItemModel();
        link->procCutJumpInit(FALSE);
        running = false;

        if (retval != nullptr) {
            *static_cast<int*>(retval) = 1;
        }

        return HOOK_SKIP_ORIGINAL;
    }
    return HOOK_CONTINUE;
}

MOD_EXPORT ModResult mod_initialize(ModError*) {
    mods::hook::add_pre<LinkProcMoveInit>(link_proc_move_init_pre);
    mods::hook::add_pre<LinkSetDoubleAnime>(link_set_double_anime_pre);
    mods::hook::add_pre<LinkCheckCutAction>(link_check_cut_action_pre);
    return MOD_OK;
}

MOD_EXPORT ModResult mod_update(ModError*) {
    daAlink_c* link = daAlink_getAlinkActorClass();
    if (running && mDoCPd_c::getHoldA(0) == 0 || (link && link->checkEventRun()) ||
        (link && link->mProcID != daAlink_c::daAlink_PROC::PROC_MOVE))
    {
        running = false;
    }
    return MOD_OK;
}

MOD_EXPORT ModResult mod_shutdown(ModError*) {
    return MOD_OK;
}
}
