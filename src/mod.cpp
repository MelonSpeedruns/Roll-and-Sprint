#include "d/actor/d_a_alink.h"
#include "d/d_camera.h"
#include "mods/service.hpp"
#include "mods/svc/hook.h"
#include "mods/svc/hook.hpp"
#include <mods/svc/ui.h>

DEFINE_MOD();
IMPORT_SERVICE(HookService, svc_hook);
IMPORT_SERVICE(UiService, svc_ui);
IMPORT_SERVICE(ConfigService, svc_config);

DEFINE_HOOK(&daAlink_c::procMoveInit, LinkProcMoveInit);
DEFINE_HOOK(&daAlink_c::setDoubleAnime, LinkSetDoubleAnime);
DEFINE_HOOK(&daAlink_c::checkNormalAction, LinkCheckCutAction);

UiElementHandle statusText = 0;
ConfigVarHandle var = 0;

bool running = false;

extern "C" {

HookAction link_proc_move_init_pre(ModContext* ctx, void* args, void* retval, void*) {
    daAlink_c* link = daAlink_getAlinkActorClass();
    if (!running && link && link->mProcID == daAlink_c::daAlink_PROC::PROC_FRONT_ROLL) {
        if (mDoCPd_c::getHoldA(0) != 0 && !link->checkEventRun() && !link->checkBootsOrArmorHeavy())
        {
            if (link->mEquipItem != 0xFF) {
                link->allUnequip(0);
            }

            link->setSwordVoiceSe(Z2SE_AL_V_THROW_IB);
            running = true;
            dCamera_c* camera = dCam_getBody();
            if (camera) {
                camera->mCamParam.mManualMode = 0;
            }
        }
    }
    return HOOK_CONTINUE;
}

HookAction link_set_double_anime_pre(ModContext* ctx, void* args, void*, void*) {
    daAlink_c* link = daAlink_getAlinkActorClass();
    if (link && running) {
        daAlink_c::daAlink_ANM& linkAnm = mods::arg_ref<daAlink_c::daAlink_ANM>(args, 5);

        if (linkAnm == link->ANM_RUN) {
            int64_t speed = 100;
            svc_config->get_int(mod_ctx, var, &speed);
            f32& anmSpeed = mods::arg_ref<f32>(args, 3);
            anmSpeed = 2.0f * (speed / 100.0f);
            linkAnm = link->ANM_RUN_B;
            link->mNormalSpeed = 35.0f * (speed / 100.0f);
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

ModResult build(ModContext*, UiElementHandle panel, void*, ModError*) {
    svc_ui->pane_add_section(mod_ctx, panel, "Settings");

    UiControlDesc control = UI_CONTROL_DESC_INIT;
    control.kind = UI_CONTROL_NUMBER;
    control.label = "Speed Multiplier (%)";
    control.help_rml = "Shown in the help pane while focused.";
    control.binding = UI_BINDING_CONFIG_VAR;
    control.config_var = var;  // from svc_config->register_var
    control.min = 80;
    control.max = 200;
    control.step = 5;
    svc_ui->pane_add_control(mod_ctx, panel, &control, &statusText);

    return MOD_OK;
}

ModResult update(ModContext*, void*, ModError*) {
    svc_ui->elem_set_text(mod_ctx, statusText, "running");
    return MOD_OK;
}

MOD_EXPORT ModResult mod_initialize(ModError*) {
    ConfigVarDesc desc = CONFIG_VAR_DESC_INIT;
    desc.name = "speedMultiplier";  // 1-64 chars from [A-Za-z0-9_-]; "enabled" is reserved
    desc.type = CONFIG_VAR_INT;
    desc.default_int = 100;
    svc_config->register_var(mod_ctx, &desc, &var);

    mods::hook::add_pre<LinkProcMoveInit>(link_proc_move_init_pre);
    mods::hook::add_pre<LinkSetDoubleAnime>(link_set_double_anime_pre);
    mods::hook::add_pre<LinkCheckCutAction>(link_check_cut_action_pre);

    UiModsPanelDesc panel = UI_MODS_PANEL_DESC_INIT;
    panel.build = build;
    panel.update = update;
    svc_ui->register_mods_panel(mod_ctx, &panel);

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
