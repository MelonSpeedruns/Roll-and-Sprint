#include "d/actor/d_a_alink.h"
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
DEFINE_HOOK(&daAlink_c::decideCommonDoStatus, LinkDecideCommonDoStatus);

UiElementHandle statusText = 0;
ConfigVarHandle speed_var = 0;
ConfigVarHandle toggle_var = 0;

bool running = false;
bool holdingA = false;

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
            holdingA = true;

            // Set here to prevent roll briefly appearing when in non-toggle mode
            link->setDoStatus(BUTTON_STATUS_NONE);
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
            svc_config->get_int(mod_ctx, speed_var, &speed);
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

void link_decide_common_do_status_post(ModContext* ctx, void* args, void* retval, void*) {
    daAlink_c* link = daAlink_getAlinkActorClass();

    bool toggle = false;
    svc_config->get_bool(mod_ctx, toggle_var, &toggle);

    // BUTTON_STATUS_UNK_121 is Roll
    if (dComIfGp_getDoStatus() == BUTTON_STATUS_UNK_121)
    {
        if (!toggle)
        {
            if (running)
            {
                link->setDoStatus(BUTTON_STATUS_NONE);
            }
        }
        else
        {
            if (running)
            {
                link->setDoStatus(BUTTON_STATUS_CANCEL);
            }
            else if (holdingA)
            {
                link->setDoStatus(BUTTON_STATUS_NONE);
            }
        }
    }
}

ModResult build(ModContext*, UiElementHandle panel, void*, ModError*) {
    svc_ui->pane_add_section(mod_ctx, panel, "Settings");

    UiControlDesc speedMultiplerControl = UI_CONTROL_DESC_INIT;
    speedMultiplerControl.kind = UI_CONTROL_NUMBER;
    speedMultiplerControl.label = "Speed Multiplier (%)";
    speedMultiplerControl.help_rml = "Shown in the help pane while focused.";
    speedMultiplerControl.binding = UI_BINDING_CONFIG_VAR;
    speedMultiplerControl.config_var = speed_var;  // from svc_config->register_var
    speedMultiplerControl.min = 80;
    speedMultiplerControl.max = 200;
    speedMultiplerControl.step = 5;
    svc_ui->pane_add_control(mod_ctx, panel, &speedMultiplerControl, &statusText);

    UiControlDesc toggleSprintControl = UI_CONTROL_DESC_INIT;
    toggleSprintControl.kind = UI_CONTROL_TOGGLE;
    toggleSprintControl.label = "Toggle Sprint";
    toggleSprintControl.help_rml = "Shown in the help pane while focused.";
    toggleSprintControl.binding = UI_BINDING_CONFIG_VAR;
    toggleSprintControl.config_var = toggle_var;  // from svc_config->register_var
    svc_ui->pane_add_control(mod_ctx, panel, &toggleSprintControl, &statusText);

    return MOD_OK;
}

ModResult update(ModContext*, void*, ModError*) {
    svc_ui->elem_set_text(mod_ctx, statusText, "running");
    return MOD_OK;
}

MOD_EXPORT ModResult mod_initialize(ModError*) {
    ConfigVarDesc speedMultiplier = CONFIG_VAR_DESC_INIT;
    speedMultiplier.name = "speedMultiplier";  // 1-64 chars from [A-Za-z0-9_-]; "enabled" is reserved
    speedMultiplier.type = CONFIG_VAR_INT;
    speedMultiplier.default_int = 100;
    svc_config->register_var(mod_ctx, &speedMultiplier, &speed_var);

    ConfigVarDesc toggleSprint = CONFIG_VAR_DESC_INIT;
    toggleSprint.name = "toggleSprint";  // 1-64 chars from [A-Za-z0-9_-]; "enabled" is reserved
    toggleSprint.type = CONFIG_VAR_BOOL;
    toggleSprint.default_bool = false;
    svc_config->register_var(mod_ctx, &toggleSprint, &toggle_var);

    mods::hook::add_pre<LinkProcMoveInit>(link_proc_move_init_pre);
    mods::hook::add_pre<LinkSetDoubleAnime>(link_set_double_anime_pre);
    mods::hook::add_pre<LinkCheckCutAction>(link_check_cut_action_pre);
    mods::hook::add_post<LinkDecideCommonDoStatus>(link_decide_common_do_status_post);

    UiModsPanelDesc panel = UI_MODS_PANEL_DESC_INIT;
    panel.build = build;
    panel.update = update;
    svc_ui->register_mods_panel(mod_ctx, &panel);

    return MOD_OK;
}

MOD_EXPORT ModResult mod_update(ModError*) {
    daAlink_c* link = daAlink_getAlinkActorClass();
    if (mDoCPd_c::getHoldA(0) == 0)
    {
        holdingA = false;
    }

    if (link)
    {
        bool toggle = false;
        svc_config->get_bool(mod_ctx, toggle_var, &toggle);

        if (!toggle)
        {
            if (running && mDoCPd_c::getHoldA(0) == 0 || link->checkEventRun() ||
                link->mProcID != daAlink_c::daAlink_PROC::PROC_MOVE)
            {
                running = false;
            }
        }
        else
        {
            if (running && !holdingA && mDoCPd_c::getHoldA(0) != 0 || link->checkEventRun() ||
                link->mProcID != daAlink_c::daAlink_PROC::PROC_MOVE || mDoCPd_c::getStickValue(0) == 0)
            {
                running = false;
                // If running is false but this is true then we want to avoid rolling
                holdingA = true;
            }
        }
    }

    return MOD_OK;
}

MOD_EXPORT ModResult mod_shutdown(ModError*) {
    return MOD_OK;
}
}
