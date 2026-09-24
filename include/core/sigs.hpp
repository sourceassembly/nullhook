/*
/^-----^\   data: 2026-04-30
V  o o  V  file: src/core/shared/sigs.hpp
 |  Y  |   author: pupnoodle
  \ Q /
  / - \
  |    \
  |     \     )
  || (___\====
*/
#ifndef SHARED_SIGS_HPP
#define SHARED_SIGS_HPP

namespace sigs
{

constexpr const char* input =
  "48 8D 05 ? ? ? ? 48 8B 38 48 8B 07 FF 90 ? ? ? ? 48 8D 15 ? ? ? ? 84 C0";
constexpr const char* move_helper =
  "48 8D 05 ? ? ? ? 48 89 85 ? ? ? ? 74 ? 48 8B 38";
constexpr const char* client_state =
  "48 8D 05 ? ? ? ? 4C 8B 40";
constexpr const char* client_state_force_full_update =
  "83 BF B8 01 00 00 FF 74 ? 55 48 89 E5 53 48 89 FB 48 83 EC 08 E8";
constexpr const char* client_mode_shared =
  "48 8D 05 ? ? ? ? 40 0F B6 F6 48 8B 38 48 8B 07 FF 60 58";

constexpr const char* in_cond =
  "55 83 FE ? 48 89 E5 41 54 41 89 F4";
constexpr const char* tfplayer_update_client_side_animation =
  "55 48 89 E5 41 54 49 89 FC 53 E8 ? ? ? ? 48 85 C0 0F 84 ? ? ? ? 48 89 C3 48 8B 00 48 89 DF FF 90 ? ? ? ? 84 C0 0F 84 ? ? ? ? 49 39 DC";
constexpr const char* cl_move =
  "55 48 89 E5 41 57 41 56 41 55 41 54 53 48 83 EC 78 83 3D ? ? ? ? 01 F3 0F 11 85 ? ? ? ? 0F 8E ? ? ? ? 41 89 FE E8 ? ? ? ? 84 C0 89 C3 0F 84 ? ? ? ? 4C 8B 3D ? ? ? ? 31";
constexpr const char* attribute_hook_value_float =
  "55 31 C0 48 89 E5 41 57 41 56 41 55 49 89 F5 41 54 49 89 FC 53 89 CB";
constexpr const char* intro_menu_on_tick =
  "55 48 89 E5 41 55 41 54 49 89 FC 48 8B BF ? ? ? ? 48 8B 07 FF 90 ? ? ? ? 84 C0";
constexpr const char* class_menu_show_panel =
  "55 48 89 E5 41 55 41 54 49 89 FC 53 89 F3 40 0F B6 F6 48 83 EC ? E8 ? ? ? ? 84 DB 48 8D 1D";
constexpr const char* team_menu_show_panel =
  "55 48 89 E5 41 56 41 55 41 54 49 89 FC 53 48 83 EC ? 40 84 F6 0F 85";
constexpr const char* key_values_constructor =
  "55 66 0F EF C0 48 89 E5 53 48 89 FB 48 89 F7 48 83 EC ? BE ? ? ? ? C7 03 FF FF FF FF";
constexpr const char* key_values_set_int =
  "55 48 89 E5 53 89 D3 BA ? ? ? ? 48 83 EC ? E8 ? ? ? ? 48 85 C0 74 ? 89 58";
constexpr const char* key_values_load_from_buffer =
  "55 48 89 E5 41 57 41 56 41 55 41 54 53 48 81 EC ? ? ? ? 48 85 D2 48 89 BD";
constexpr const char* key_values_delete_this =
  "48 85 FF 74 3B 55 48 89 E5 41 54 49 89 FC 48 83 EC 08 E8 ? ? ? ? 41 80 7C 24 23 00 74 ? 4C 89 E7 E8 ? ? ? ? E8 ? ? ? ? 4C 89 E6";
constexpr const char* random_seed =
  "48 8D 05 ? ? ? ? BA ? ? ? ? 89 10";
constexpr const char* cam_cap_yaw =
  "55 48 89 E5 41 54 48 83 EC 18 F3 0F 11 45 EC E8 ? ? ? ? 48 85 C0 74 ? 4C 8D A0 ? ? ? ? BE 11 00 00 00";

constexpr const char* casual_rank_record =
  "55 48 89 E5 41 54 53 48 89 FB 48 83 EC 10 48 8B 87 70 02 00 00 80 B8 B0 00 00 00 00 0F 84 ? ? ? ? 8B 90 AC 00 00 00 48 8D 3D ? ? ? ? 40 84 F6 0F 45 90 A8 00 00 00 41 89 D4";
constexpr const char* item_schema_lookup_map =
  "48 8B 05 ? ? ? ? 55 48 89 E5 41 55 41 54 48 85 C0 74 ? 4C 8D 60 ?";
constexpr const char* item_definition_lookup =
  "55 48 89 E5 41 55 41 54 49 89 FC 53 48 83 EC ? 8B 87 ? ? ? ? 85 C0 0F 84 ? ? ? ? 41 89 F1";
constexpr const char* attribute_definition_lookup =
  "48 B8 ? ? ? ? ? ? ? ? 55 66 0F EF C0 48 89 E5 41 57 41 56 41 55 41 54 53 48 83 EC 78 44 8B A7 68 02 00 00";
constexpr const char* attribute_list_set_runtime_value =
  "55 48 89 E5 41 57 41 56 49 89 F6 41 55 66 41 0F 7E C5 41 54 49 89 FC 53 48 83 EC 28 8B 57 18 85 D2 7E ? 48 8B 5F 08";
constexpr const char* inventory_find_item_by_id =
  "44 8B 4F ? 49 89 FA 49 89 F0 49 89 D3 45 85 C9 7E ? 48 8B 47 ? 31 F6 31 C9 EB ?";
constexpr const char* inventory_find_item_by_def =
  "55 48 89 E5 41 57 41 56 41 55 41 54 53 48 83 EC ? 4C 63 67 ? 45 85 E4 7E ? 4D 69 E4 ? ? ? ? 49 89 FD 41 89 F6";
constexpr const char* tf_weapon_base_gun_get_bullet_spread =
  "55 31 D2 48 89 FE B9 ? ? ? ? 48 89 E5 41 54 53 48 89 FB 48 83 EC ? 48 63 87 ? ? ? ? 48 C1 E0 ?";
constexpr const char* ctf_weapon_base_calc_is_attack_critical =
  "55 48 89 E5 41 57 41 56 41 55 41 54 49 89 FC 53 48 83 EC ? E8 ? ? ? ? 48 85 C0 0F 84 ? ? ? ? 48 8D 15 ? ? ? ? 31 C9 48 89 C7 48 8D 35 ? ? ? ? E8 ? ? ? ? 48 85 C0 49 89 C5 0F 84 ? ? ? ? 48 8B 00 4C 89 EF FF 90 ? ? ? ? 84 C0";
constexpr const char* ctf_weapon_base_melee_calc_is_attack_critical =
  "55 48 89 E5 41 57 41 56 41 55 41 54 49 89 FC 53 48 83 EC ? E8 ? ? ? ? 48 85 C0 74 ? 48 89 C3 48 8B 00 48 89 DF FF 90 ? ? ? ? 84 C0 74 ? 49 8B 04 24 31 D2 31 F6 4C 89 E7 FF 90 ? ? ? ? 84 C0";
constexpr const char* ctf_weapon_base_melee_calc_is_attack_critical_helper =
  "48 8B 05 ? ? ? ? 8B 40 58 85 C0 75 ? 48 8B 07 FF A0 ? ? ? ?";
constexpr const char* ctf_weapon_base_calc_is_attack_critical_outer =
  "55 48 89 E5 41 55 41 54 53 48 89 FB 48 83 EC 08 E8 ? ? ? ? 48 85 C0 0F 84 ? ? ? ? 48 8D 15 ? ? ? ? 31 C9 48 89 C7 48 8D 35 ? ? ? ? E8 ? ? ? ? 48 85 C0 49 89 C4 74 ? 48 8B 00 4C 89 E7 FF 90 ? ? ? ? 84 C0 74 ? 48 8D 05 ? ? ? ? 48 8B 00 8B 40 04";
constexpr const char* cinput_validate_usercmd =
  "55 48 89 E5 41 56 41 89 D6 41 55 49 89 FD 41 54 4C 8D 65 DC 53 4C 89 E7 48 89 F3 48 83 EC 10 E8 ? ? ? ? 48 8D 73 08 BA 04 00 00 00 4C 89 E7 E8 ? ? ? ? 48 8D 73 0C BA 04 00 00 00 4C 89 E7";
constexpr const char* ctf_weapon_base_can_fire_random_critical_shot =
  "F3 0F 58 05 ? ? ? ? 0F 2F 87 ? ? 00 00 0F 93 C0 C3";
constexpr const char* ctf_player_anim_state_store =
  "E8 ? ? ? ? 48 8B 7D ? 49 89 84 24 ? ? ? ? 4C 89 E6 E8 ? ? ? ? 49 8D B4 24 ? ? ? ?";

constexpr const char* base_animating_invalidate_bone_cache =
  "48 8B 05 ? ? ? ? C7 87 ? ? ? ? FF FF 7F FF 48 83 E8 01 48 89 87 ? ? ? ? C3";
constexpr const char* studio_get_bone_cache =
  "55 48 8D 0D ? ? ? ? 48 89 E5 41 56 41 55 4C 8D 35 ? ? ? ? 49 89 FD 41 54 4C 8D 25 ? ? ? ? 66 49 0F 6E CE 53 66 49 0F 6E C4 48 83 EC 30 48 8B 1D ? ? ? ? 66 0F 6C C1 48 89 4D C0 0F 29 45 B0 C7 45 C8 DC 00 00 00";
constexpr const char* studio_bone_cache_update_bones =
  "55 48 89 E5 41 57 41 56 41 55 41 54 53 48 89 FB 48 83 EC 18 44 0F B7 7F 0E F3 0F 11 45 CC 0F B7 47 10";
constexpr const char* base_animating_auto_allow_bone_access =
  "44 0F B6 C2 40 0F B6 FE BA 01 00 00 00 44 89 C6 E9 ? ? ? ?";
constexpr const char* base_animating_auto_allow_bone_access_on_delete =
  "55 BF 01 00 00 00 48 89 E5 E8 ? ? ? ? 5D C3";

constexpr const char* base_animating_bone_handle =
  "55 48 89 E5 41 56 49 89 F6 41 55 49 89 FD 41 54 53 48 83 EC 20 48 8B BF ? ? ? ?";

constexpr const char* ik_context_clear_targets =
  "8B 8F ? ? ? ? 48 8D 97 ? ? ? ? 31 C0 85 C9 7E ? 0F 1F 44 00 00 C7 02 F1 D8 FF FF 83 C0 01 48 81 C2 60 01 00 00 39 87 ? ? ? ? 7F ? C3 90";
constexpr const char* base_animating_add_eflags =
  "09 B7 ? ? ? ? C3";
constexpr const char* base_animating_ik =
  "4C 8B 83 ? ? ? ? 72";
constexpr const char* base_animating_studio_hdr =
  "4C 8B BB ? ? ? ? 4D 85 FF 0F 84 ? ? ? ? 4C 89 FF E8";
constexpr const char* base_animating_anim_overlay =
  "89 F0 48 6B C0 2C 48 03 87 ? ? ? ? C3";
constexpr const char* base_animating_bone_array =
  "48 8B B3 ? ? ? ? 48 8D 14 52";

constexpr const char* inspect_target_check =
  "55 48 89 E5 41 55 41 54 53 48 81 EC ? ? ? ? 48 85 F6 74 ? 48 8D 05 ? ? ? ?";
constexpr const char* tf_inventory_manager_initializer =
  "55 48 8D 3D ? ? ? ? 48 89 E5 E8 ? ? ? ? 48 8D 15 ? ? ? ? 48 8D 35 ? ? ? ? 48 8D 3D ? ? ? ? E8 ? ? ? ? 48 8D 15 ? ? ? ?";

constexpr const char* get_party_client =
  "48 8D 05 ? ? ? ? C3 0F 1F 84 00 00 00 00 00 48 8B 05 ? ? ? ? C3";
constexpr int get_party_client_offset = 16;
constexpr const char* get_matchmaking_client =
  "48 8D 05 ? ? ? ? C3 0F 1F 84 00 00 00 00 00 48 8B 05 ? ? ? ? C3 0F 1F 84 00 00 00 00 00 48 8D 05 ? ? ? ? 48 8B 00 48 85 C0 74 ? 55";
constexpr const char* tf_gc_client_system_ping_think =
  "55 48 89 E5 41 57 41 56 41 55 49 89 FD 41 54 48 8D 3D ? ? ? ? 53 48 81 EC ? ? ? ?";
constexpr const char* tf_gc_client_system_so_event =
  "55 48 89 E5 41 57 41 56 49 89 FE 48 89 F7 41 55 41 89 D5 41 54 49 89 F4 53 48 83 EC ? 48 8B 06 FF 50 10 3D D4 07 00 00 0F 84 ? ? ? ? 49 8B 04 24 4C 89 E7 FF 50 10 83 F8 2A 74 ? 49 8B 04 24 4C 89 E7 FF 50 10 3D D8 07 00 00";
constexpr const char* tf_gc_client_system_get_match_invite =
  "55 48 89 E5 41 55 45 31 ED 41 54 49 89 FC 53 48 63 DE 48 83 EC ? 48 8B BF ? ? ? ? 48 85 FF 74 ? BE D8 07 00 00";
constexpr const char* tf_gc_client_system_request_accept_match_invite =
  "48 83 BF ? ? ? ? 00 74 ? C3 0F 1F 44 00 00 55 48 89 E5 41 57 49 89 F7 41 56 41 55 41 54 49 89 FC BF ? ? ? ? 53 48 83 EC ? E8 ? ? ? ? 49 89 C6 E8 ? ? ? ? 48 89 C7 E8 ? ? ? ? 48 8D 70 ?";
constexpr const char* tf_gc_client_system_join_mm_match =
  "55 48 89 E5 41 55 41 54 0F B6 87 CE 07 00 00 49 89 FC 89 C2 81 E2 F0 00 00 00 0F 84 ? ? ? ? 3C AF 0F 87 ? ? ? ? 0F B6 87 CF 07 00 00 83 E8 01 3C 03 0F 87 ? ? ? ? 80 FA 10 74 ? 80 FA 70 0F 84 ? ? ? ? 80 FA 30 75 ? 8B 87 C8 07 00 00 85 C0 0F 84";

constexpr const char* mut_local_group_criteria =
  "48 83 7F 30 00 74 06 80 7F 40 00 74 0B 48 8D 87 ? ? ? ? C3";
constexpr const char* group_criteria_set_match_group =
  "89 77 2C C3 66 66 2E 0F 1F 84 00 00 00 00 00 90 89 77 30 C3";
constexpr int group_criteria_set_match_group_offset = 16;
constexpr const char* load_saved_casual_criteria =
  "48 83 7F 30 00 C6 87 10 03 00 00 01 74 ? 80 7F 40 00 74 ? C6 87 30 03 00 00 01 48 8D 35 ? ? ? ? 48 81 C7 B0 01 00 00 E9 ? ? ? ?";
constexpr const char* is_in_queue_for_match_group =
  "55 48 89 E5 41 54 49 89 FC 89 F7 53 89 F3 E8 ? ? ? ? 83 FB FF 41 89 C0 0F 94 C0 41 83 F0 01 41 08 C0 75 ? 41 8B 54 24 58 85 D2";
constexpr const char* is_in_standby_queue =
  "0F B6 47 68 C3 90 66 2E 0F 1F 84 00 00 00 00 00 55 31 F6";
constexpr const char* abandon_current_match =
  "55 31 C0 48 89 E5 41 55 41 54 4C 8D 65 ? 53 48 89 FB 48 8D 3D ? ? ? ? 48 83 EC 38 E8 ? ? ? ? 4C 89 E7 BE 91 18 00 00 E8 ? ? ? ? 48 8B 45 ? 48 89 DF 83 48 10 01 C6 40 28 01 4C 8B 6D ? E8 ? ? ? ? 48 85 C0 0F 84 ? ? ? ? 48 89 DF E8 ? ? ? ? 48 8D 0D ? ? ? ? 48 89 C7 48 8B 00 48 8B 50 10 48 39 CA";
constexpr const char* request_queue_for_match =
  "55 48 89 E5 41 57 41 56 49 89 FE 89 F7 41 55 41 54 41 89 F4 53 48 81 EC 88 00 00 00 E8 ? ? ? ? 41 83 FC FF 0F 94 C3 3C 01 75 ? 84 DB 75 ? 49 63 C4 41 80 BC 06 1E 03 00 00 00 75 ?";
constexpr const char* request_leave_for_match =
  "55 48 89 E5 41 57 41 56 41 55 49 89 FD 41 54 53 48 63 DE 48 83 EC ? 89 DE E8 ? ? ? ? 84 C0 75 ? 48 83 C4 ? 5B 41 5C 41 5D 41 5E 41 5F 5D";
constexpr const char* request_queue_for_standby =
  "48 83 7F 30 00 0F 84 ? ? ? ? 55 48 89 E5 41 57 41 56 41 55 41 54 49 89 FC 53 48 83 EC 38 80 BF 12 03 00 00 00 74 ?";
constexpr const char* request_leave_standby =
  "80 7F 68 00 0F 84 ? ? ? ? 55 48 89 E5 41 57 41 56 41 55 41 54 53 48 89 FB 48 83 EC 38 80 BF 13 03 00 00 00 74 ?";
constexpr const char* party_client_get_num_members =
  "48 8B 7F 30 48 85 FF 74 ? 48 8B 07 48 8D 15 ? ? ? ? 48 8B 40 78 48 39 D0 75 ? 8B 47 38 C3";
constexpr const char* party_client_get_num_online_members =
  "55 48 89 E5 41 57 41 56 41 55 41 54 53 48 83 EC 18 48 8B 5F 30 48 85 DB 0F 84 ? ? ? ? 49 89 FD 48 8B 7F 28 48 C7 45 C8 00 00 00 00";
constexpr const char* party_client_get_member_steamid =
  "55 48 89 E5 41 54 48 83 EC 18 4C 8B 67 30 4D 85 E4 74 ? 49 8B 04 24 48 8D 15 ? ? ? ? 48 8B 40 78 48 39 D0 75 ? 41 8B 44 24 38 85 F6 78 ? 39 F0 7E ? 49 8B 04 24 4C 89 E7 4C 8B 65 F8 48 8B 80 80 00 00 00 C9 FF E0";
constexpr const char* party_client_in_party_not_leader =
  "31 C0 48 83 7F 30 00 74 ? 0F B6 47 40 83 F0 01 C3";
constexpr const char* party_client_send_party_chat =
  "55 48 89 E5 41 57 41 56 41 55 49 89 FD 48 89 F7 41 54 49 89 F4 53 48 83 EC 58 E8 ? ? ? ? 3D FF 00 00 00 0F 8F ? ? ? ? 49 83 7D 30 00 0F 84 ? ? ? ?";
constexpr const char* party_client_kick_player =
  "55 48 89 E5 41 56 41 55 41 54 53 48 89 FB 48 83 EC 40 48 8B 7F 30 48 89 75 A8 48 85 FF 74 ? 44 0F B6 63 40 45 84 E4 75 ? 48 83 C4 40 45 31 E4 5B 44 89 E0 41 5C 41 5D 41 5E 5D C3 ? ? ? ? 48 8B 07 4C 8D 75 A8 4C 89 F6 FF 90 88 00 00 00 83 F8 FF 74 ? 4C 8D 6D B0 BE B0 19 00 00";
constexpr const char* promote_to_leader =
  "55 48 89 E5 41 56 41 55 41 54 53 48 89 FB 48 83 EC 40 48 8B 7F 30 48 89 75 A8 48 85 FF 74 ? 44 0F B6 63 40 45 84 E4 75 ? 48 83 C4 40 45 31 E4 5B 44 89 E0 41 5C 41 5D 41 5E 5D C3 0F 1F 40 00 48 8B 07 4C 8D 75 A8 4C 89 F6 FF 90 ? ? ? ? 83 F8 FF 74 ? 4C 8D 6D B0 BE AF 19 00 00";
constexpr const char* report_player_account =
  "55 48 89 F8 48 89 E5 48 C1 E8 ? 41 57 41 56 41 55 41 54 53 48 83 EC ?";
constexpr const char* report_player_recent_check =
  "55 48 89 E5 41 54 48 83 EC ? 8B 15 ? ? ? ? 85 D2 0F 8E ? ? ? ? 48 8B 05 ? ? ? ?";
constexpr const char* launcher_source_lock =
  "55 48 89 E5 41 55 41 54 4C 8D AD ? ? ? ? 48 81 EC ? ? ? ? E8 ? ? ? ?";
constexpr const char* sdl_create_window_flags =
  "41 B9 0A 00 00 00 45 84 E4 0F 84 ? ? ? ? 4C 89 FF 45 89 E8 44 89 F1 BA 00 00 FF 2F BE 00 00 FF 2F E8";
constexpr const char* sdl_show_window_resize =
  "48 8B 7B 28 E8 ? ? ? ? 48 8B 7B 28 E8 ? ? ? ? C6 43 45 01 48 83 C4 08";
constexpr const char* sdl_show_window_after_create =
  "48 8B 7B 28 88 45 BC E8 ? ? ? ? 0F B6 45 BC";
constexpr const char* sdl_show_window_present =
  "48 8B 7B 28 E8 ? ? ? ? 48 8B 7B 28 E8 ? ? ? ? C6 43 45 01 E9";
constexpr const char* video_mode_setup_startup_graphic =
  "55 31 C0 48 89 E5 41 57 41 56 4C 8D B5 ? ? ? ? 41 55 41 54 4C 8D A5 ? ? ? ? 53 48 89 FB 48 8D 3D ? ? ? ? 48 81 EC ? ? ? ? E8 ? ? ? ? 31 D2 BE ? ? ? ? 4C 89 F7";
constexpr const char* client_file_system =
  "31 F6 4C 89 EF FF 13 48 83 3D ? ? ? ? 00 48 89 05 ? ? ? ? 0F 85";
constexpr const char* v_render_view =
  "55 31 C0 48 89 E5 41 56 41 55 41 54 53 48 83 EC 40 4C 8B 2D ? ? ? ? 48 C7 45 A0 00 00 00 00 49 8B 7D 18 48 85 FF 74 ? 48 83 EC 08 45 31 C0 31 C9 48 8D 05 ? ? ? ? 31 D2";
constexpr const char* vox_shutdown_bad_vcall =
  "48 8D 3D ? ? ? ? 31 F6 E8 ? ? ? ? E8 ? ? ? ? 48 8B 3D ? ? ? ? 48 85 FF 74 06 48 8B 07 FF 50 08";
constexpr const char* material_system_swap_buffers =
  "55 31 C0 48 89 E5 41 56 41 55 41 54 53 48 89 FB 48 83 EC 20 4C 8B 25 ? ? ? ? 48 C7 45 C8 00 00 00 00 49 8B 7C 24 10 48 85 FF 74 50";
constexpr const char* particle_property_create =
  "55 48 89 E5 41 57 41 56 49 89 F6 41 55 41 54 53 48 89 FB 48 83 EC ? 48 8D 05 ? ? ? ? 89 55 ? 89 4D ? 66 0F D6 45 ?";
constexpr const char* view_render_perform_screen_space_effects =
  "55 48 89 E5 41 57 41 56 41 89 CE 41 55 41 89 D5 41 54 41 89 F4 53 44 89 C3 48 83 EC 48";
constexpr const char* view_render_perform_screen_overlay =
  "55 31 C0 48 89 E5 41 57 41 56 41 55 45 89 C5 41 54 53 48 89 FB 48 83 EC 68";

constexpr const char* play_sequence =
  "48 85 F6 0F 84 ? ? ? ? 55 48 89 E5 41 57 41 56 41 55 41 54 49 89 FC 53 48 83 EC 28 83 BF 00 ? ? ? ?";
constexpr const char* menu_model_anim_events =
  "55 48 89 E5 41 57 49 89 FF 48 89 F7 41 56 89 D6 41 89 D6 41 55 4D 89 C5 41 54 53 89 CB 48 83 EC 28 F3 0F 11 45 CC 48 89 7D C0 E8 ? ? ? ? 8B 50 18";
constexpr const char* menu_model_update =
  "55 48 89 E5 41 57 41 56 41 55 41 54 53 48 83 EC 18 4C 8B AF 10 0C 00 00 4D 85 ED 0F 84 ? ? ? ? 49 89 FC";
constexpr const char* menu_model_apply_sequence =
  "55 48 89 E5 41 57 41 56 41 55 41 54 49 89 FC 53 48 83 EC 28 48 8D 05 ? ? ? ? 48 89 75 C8 48 8B 38 48 8B 07 48 89 7D B8 FF 90 C8 00 00 00 4D 8B BC 24 10 0C 00 00";
constexpr const char* menu_item_model_set =
  "55 48 89 E5 41 57 41 56 41 55 41 54 49 89 F4 53 48 89 FB 48 83 EC 68 C7 87 E4 0E 00 00 00 00 00 00 48 8B BF 10 0F 00 00";
constexpr const char* html_createbrowser_gate_a =
  "41 C7 84 24 18 07 00 00 00 00 00 00 4C 89 F7 E8 21 C0 FF FF 49 8B BC 24 F0 06 00 00 48 85 FF";
constexpr const char* html_createbrowser_gate_b =
  "41 C7 84 24 18 07 00 00 00 00 00 00 4C 89 F7 E8 A1 9D FF FF 49 8B BC 24 F0 06 00 00 48 85 FF";
constexpr const char* particle_system_precache =
  "31 C0 48 85 FF 74 ? 55 48 89 E5 41 54 49 89 FC 48 83 EC ? 48 8B 3D ? ? ? ?";
constexpr const char* particle_effect_create_event =
  "55 41 89 C9 48 89 E5 41 57 41 56 41 55 4D 89 C5 41 54 49 89 FC 53 48 81 EC ? ? ? ?";
constexpr const char* view_render_render =
  "55 31 C0 48 89 E5 41 57 49 89 FF 41 56 41 55 41 54 53 48 81 EC ? ? ? ? 48 8B 1D ? ? ? ? 48 89 B5 ? ? ? ? 48 C7 85 ? ? ? ? ? ? ? ?";
constexpr const char* client_objective_flag_countdown_update =
  "49 8B BC 24 ? ? ? ? 48 8B 07 FF 90 ? ? ? ? 84 C0 0F 85 ? ? ? ? 49 8B 06 48 85 C0 74 ?";
constexpr const char* client_panel_image_paint =
  "55 48 89 E5 41 56 41 55 41 54 49 89 FC 53 48 83 EC ? 80 BF ? ? ? ? 00 0F 85 ? ? ? ? 4D 8B AC 24 ? ? ? ? 4D 85 ED 0F 84 ? ? ? ? 49 8B 45 00 48 8D 1D ? ? ? ? 4C 8B 70 28";
constexpr const char* server_scene_entity_should_transmit =
  "55 48 89 E5 41 57 41 56 49 89 F6 41 55 41 54 49 89 FC 53 48 83 EC 08 E8 ? ? ? ? 49 8B BC 24 98 08 00 00 41 89 C7 48 85 FF 74 ? 31 DB 83 F8 10";
constexpr const char* server_base_entity_should_transmit =
  "55 48 89 E5 41 55 41 54 49 89 FC 53 48 89 F3 48 83 EC 08 E8 ? ? ? ? A8 20 75 ? A8 08 0F 85 ? ? ? ? A8 10 41 B8 10 00 00 00";
constexpr const char* load_named_skys =
  "48 8D 05 ? ? ? ? 55 48 8D 15 ? ? ? ? 66 48 0F 6E C8 48 89 E5 41 57 48 8D 05 ? ? ? ? 41 56 66 48 0F 6E C2 41 55";

constexpr const char* mvm_completed_tour_mask =
  "55 48 89 E5 41 57 41 56 49 89 F6 41 55 49 89 D5 41 54 41 89 FC 53 48 83 EC 18 C7 06 00 00 00 00 C7 02 00 00 00 00";
constexpr const char* protobuf_repeated_field_reserve =
  "8B 47 10 39 F0 7D ? 55 53 48 89 FB 48 83 EC 08 83 FE 04";
constexpr const char* protobuf_string_new_element =
  "48 83 EC 08 BF 20 00 00 00 E8 ? ? ? ? 48 8D 50 10 48 C7 40 08 00 00 00 00 48 89 10 C6 40 10 00";

constexpr const char* hud_find_element =
  "55 48 89 E5 41 57 41 56 41 55 41 54 49 89 F4 53 48 83 EC 08 8B 47 30 85 C0 7E ?";
constexpr const char* hud_mouse_sensitivity =
  "F3 0F 10 48 ? F3 0F 11 50 ?";
constexpr const char* hud_instance_from_chat =
  "55 48 8D 35 ? ? ? ? 48 89 E5 41 54 53 48 83 EC 70 48 8D 3D ? ? ? ? E8 ? ? ? ?";
constexpr const char* gamerules_recvproxy =
  "48 8D 05 ? ? ? ? 48 8B 00 48 89 06 C3 66 90 C3 90";
constexpr const char* screenspace_effects_manager =
  "48 8D 05 ? ? ? ? 41 89 D8 44 89 F1 44 89 EA 44 89 E6 48 8B 38";
constexpr const char* screenspace_registration_head =
  "55 48 89 E5 41 54 49 89 F4 53 48 8B 1D ? ? ? ? 48 85 DB 75 ?";

constexpr const char* fx_fire_bullets =
  "55 48 89 E5 41 57 49 89 FF 44 89 C7 41 56 41 55 41 54 45 89 C4 53 48 89 D3 48 81 EC D8 00 00 00 8B 45 10 89 B5 ? ? ? ? 48 89 8D ? ? ? ?";
constexpr const char* write_usercmd =
  "55 48 89 E5 41 55 49 89 D5 41 54 49 89 FC 53 48 89 F3 48 83 EC 08 8B 72 08 8B 4F 10 8B 7F 0C 8D 56 01 39 53 08 89 F8 0F 84 ? ? ? ? 39 F9";
constexpr const char* get_projectile_fire_setup =
  "55 48 89 E5 41 57 45 89 C7 41 56 49 89 CE 41 55 49 89 F5 41 54 49 89 FC 53 48 89 D3 48 8D 15 ? ? ? ? 48 81 EC A8 01 00 00 48 8B 07 66 0F D6 85 ? ? ? ? F3 0F 10 BD ? ? ? ?";
constexpr const char* calculate_charge_cap =
  "55 48 8B B7 ? ? ? ? 31 D2 B9 01 00 00 00 F3 0F 10 05 ? ? ? ? 48 89 E5 48 8D 3D ? ? ? ? E8 ? ? ? ? 48 8B 05 ? ? ? ? 8B 40 58";
constexpr const char* get_spread_angles =
  "55 48 89 E5 41 54 53 48 89 FB 48 83 EC 50 E8 ? ? ? ? 48 85 C0 0F 84 ? ? ? ? 48 8D 15 ? ? ? ? 31 C9 48 89 C7 48 8D 35 ? ? ? ? E8 ? ? ? ? 48 85 C0 49 89 C4 0F 84 ? ? ? ?";

constexpr const char* mark_surrounding_bounds_dirty =
  "55 48 89 E5 41 54 53 48 89 FB 48 83 EC 10 48 8B 47 08 81 88 10 02 00 00 00 40 00 00";
constexpr const char* collision_update_partition =
  "55 48 89 E5 53 48 89 FB 48 83 EC 08 48 8B 7F 08 48 8B 07 FF 90 ? ? ? ? 85 C0 74 ? 48 8B 7B 08 8B 87 ? ? ? ? F6 C4 80 74 ?";

constexpr const char* cl_sendmove =
  "55 66 0F EF C0 48 89 E5 41 57 41 56 48 8D BD E8 EF FF FF 41 55 41 54 53 48 81 EC 38 10 00 00 44 8B 2D ? ? ? ?";
constexpr const char* netchan_get_sequence_data =
  "8B 47 ? 89 06 8B 47 ? 89 02 8B 47 ? 89 01 C3";
constexpr const char* netchan_set_choked =
  "83 47 ? 01 83 47 ? 01 C3";
constexpr const char* netchan_get_reliable =
  "48 85 F6 74 05 8B 47 ? 89 06 48 85 D2 74 05 8B 47 ? 89 02 48 85 C9 74 05 8B 47 ? 89 01";
constexpr const char* get_mm_ban_data =
  "83 FF FF 0F 84 ? ? ? ? 55 48 89 E5 41 55 49 89 F5 41 54 49 89 D4 53 89 FB 48 83 EC 08 E8";
constexpr const char* demo_player =
  "48 8B B7 18 04 00 00 BA 04 01 00 00 48 8D 3D ? ? ? ? E8 ? ? ? ? 48 8B 05 ? ? ? ? 48 8D 35 ? ? ? ? 48 89 05 ? ? ? ? 83 3B 01";

constexpr const char* base_animating_frame_advance =
  "80 BF DD 0B 00 00 00 0F 85 A3 03 00 00 55 48 89 E5 41 56 41 55 41 54 49 89 FC 53 48 83 EC 20";
constexpr const char* base_player_should_interpolate =
  "48 3B 3D ? ? ? ? 74 3F 55 48 89 E5 41 54 49 89 FC 48 83 EC 08 E8 ? ? ? ?";
constexpr const char* tf_player_should_draw =
  "55 48 89 E5 41 54 49 89 FC 53 E8 ? ? ? ? 84 C0 74 0B 41 80 BC 24 A8 1C 00 00 00";
constexpr const char* base_player_should_draw =
  "55 48 89 E5 41 54 49 89 FC 48 83 EC 08 E8 ? ? ? ? 84 C0 75 0A 4C 8B 65 F8 31 C0 C9 C3 66 90 4C 89 E7 4C 8B 65 F8 C9 E9 ? ? ? ? 0F 1F 00";
constexpr const char* base_entity_should_draw =
  "80 BF DD 0B 00 00 00 74 07 31 C0 C3 0F 1F 40 00 E9 ? ? ? ?";
constexpr const char* base_animating_should_draw =
  "55 48 8D 15 ? ? ? ? 48 89 E5 41 54 49 89 FC 48 83 EC 08 48 8B 07 48 8B 80 00 09 00 00 48 39 D0 75 ? 48 8D 87 30 0C 00 00";
constexpr const char* econ_wearable_should_draw =
  "8B 97 54 07 00 00 48 8D 05 ? ? ? ? 85 D2 48 8B 08 0F 84 ? ? ? ? 83 FA FF 0F B7 C2 BE FF 1F 00 00 48 0F 44 C6 C1 EA 10 48 C1 E0 05 48 01 C8 39 50 10 0F 85 ? ? ? ? 55 48 89 E5 41 55 41 54 4C 8B 68 08 4D 85 ED 74 ? 49 8B 45 00 49 89 FC 4C 89 EF FF 90 C8 05 00 00 84 C0 74 ? 4C 89 EF";
constexpr const char* tf_wearable_should_draw =
  "55 48 89 E5 41 57 41 56 41 55 41 54 49 89 FC 53 48 83 EC 18 8B 97 54 07 00 00 48 8D 1D ? ? ? ? 85 D2 48 8B 0B";
constexpr const char* interpolate_timedemo_call =
  "FF 90 70 02 00 00 84 C0 0F 85 ? ? ? ? 49 8B 3C 24 48 8B 07 FF 90 A0 02 00 00";
constexpr const char* stealth_kill_notice =
  "48 8B 06 48 8D 35 ? ? ? ? FF 50 30 84 C0 74 ?";

constexpr const char* get_match_group_description =
  "48 63 07 83 F8 08 77 ? 48 8D 15 ? ? ? ? 48 8B 04 C2 C3";
constexpr const char* register_match_desc =
  "48 63 17 48 8D 05 ? ? ? ? 48 89 34 D0 C3";
constexpr const char* match_desc_force_client_settings =
  "66 45 89 54 24 73";
constexpr const char* client_textmode_flag_store =
  "FF 50 50 85 C0 74 ? C6 05";
constexpr const char* load_white_list =
  "55 48 89 E5 41 55 41 54 49 89 FC 48 83 EC ? 48 8B 07 FF 50";
constexpr const char* allow_secure_servers_flag_ref =
  "48 8D 05 ? ? ? ? 4C 89 E7 C6 00 00 4C 8B 65 ? C9 E9 ? ? ? ?";
constexpr const char* host_is_secure_server_allowed =
  "55 48 89 E5 E8 ? ? ? ? 48 8D 35 ? ? ? ? 48 89 C7 48 8B 00 FF 50 50 85 C0 74 ? 31 C0 5D C6 05 ? ? ? ? 00 C3 0F 1F 84 00 00 00 00 00 E8 ? ? ? ? 48 8D 35 ? ? ? ? 48 89 C7 48 8B 00 FF 50 50 85 C0 75 ? 0F B6 05 ? ? ? ? 5D C3";

constexpr const char* crafting_panel_craft =
  "55 48 8D 87 ? ? ? ? 48 89 E5 41 57 49 89 FF 41 56 41 55 4C 8D 6D 80 41 54 53 48 8D 9F ? ? ? ? 48 83 EC 68 48 C7 45 80 ? ? ? ?";
constexpr const char* tf_weapon_info_primary_data =
  "48 C7 83 ? ? ? ? 00 00 00 00 48 89 03 B8 01 00 00 00 48 C7 83";
constexpr const char* store_do_preview_item =
  "55 31 C0 48 89 E5 41 56 41 55 41 54 4C 8D A5 ? ? ? ? 53 0F B7 DE 4C 89 E7 48 8D 35 ? ? ? ? 48 81 EC 10 01 00 00 89 DA E8 ? ? ? ?";
constexpr const char* client_state_process_print =
  "55 31 C0 48 89 E5 41 56 41 55 41 54 53 48 89 F3 48 83 EC 20 4C 8B 25 ? ? ? ? 48 C7 45 C8 ? ? ? ? 49 8B 7C 24 18 48 85 FF 74 ? 48 83 EC 08 45 31 C0 31 C9 48 8D 05 ? ? ? ? 31 D2 50 48 8D 05 ? ? ? ? 50 48 8D 05 ? ? ? ? 50 48 8D 05 ? ? ? ? 50 48 8D 75 C8 31 C0 68 98 04 00 00 4C 8D 0D ? ? ? ? FF 97 ? ? ? ? 49 8B 7C 24 18 48 83 C4 30 48 8B 45 C8";
constexpr const char* econ_item_schema_init_text_buffer =
  "55 48 89 E5 41 57 49 89 F7 41 56 41 55 4C 8D AD ? ? ? ? 41 54 49 89 FC 4C 89 EF 53 48 89 D3 48 81 EC C8 00 00 00 E8 ? ? ? ?";
constexpr const char* dispatch_effect =
  "55 48 89 E5 41 55 41 54 49 89 FC 53 48 83 EC 08 48 8B 1D ? ? ? ? 48 85 DB 74 ? 49 89 F5 EB ? 0F 1F 80 ? ? ? ? 48 8B 5B 10";
constexpr const char* fx_tracer =
  "55 31 C0 48 89 E5 41 57 41 56 41 89 D6 41 55 49 89 F5 41 54 49 89 FC 53 89 CB";
constexpr const char* get_particle_system_name_from_index =
  "55 48 89 E5 41 54 41 89 FC 48 83 EC 08 48 8B 3D ? ? ? ? 48 8B 07 FF 50 28 44 39 E0 7E ? 48 8B 3D ? ? ? ? 44 89 E6 4C 8B 65 F8 48 8B 07";
constexpr const char* particle_effect_callback =
  "55 48 89 E5 41 57 41 56 41 55 41 54 53 48 89 FB 48 83 EC 48 E8 ? ? ? ? 84 C0 74 ? 48 83 C4 48 5B 41 5C 41 5D 41 5E 41 5F";
constexpr const char* update_local_player_vision_flags =
  "55 C7 05 ? ? ? ? 00 00 00 00 48 89 E5 41 54 53 C7 05 ? ? ? ? 00 00 00 00 E8 ? ? ? ? 48 85 C0 74 ?";
constexpr const char* hud_upgrade_panel_cancel_upgrades =
  "55 48 89 E5 41 57 41 56 41 55 41 54 53 48 89 FB 48 83 EC 28 48 8D 05 ? ? ? ? C6 45 C7 01 48 8B 00 48 85 C0 74 ? 83 B8 ? ? ? ? ? 0F 9E 45 C7";
constexpr const char* hud_death_notice_draw_text =
  "55 48 89 E5 41 56 45 89 C6 41 55 41 89 CD 41 54 4D 89 CC 53 48 8D 1D ? ? ? ? 48 8B 3B 48 8B 07 FF 90 ? ? ? ? 48 8B 3B 44 89 F6 48 8B 07";
constexpr const char* can_report_player =
  "55 48 89 E5 41 54 48 83 EC 08 8B 15 ? ? ? ? 85 D2 0F 8E ? ? ? ? 48 8B 05 ? ? ? ? 83 EA 01 48 8D 14 52 48 8D 54 90 0C EB ? 0F 1F 00 48 83 C0 0C";

constexpr const char* party_allowed_to_party_with =
  "55 48 89 E5 48 83 EC 10 48 8B 7F 30 48 89 75 F8 48 85 FF 74 ? 48 8B 07 48 8D 75 F8 FF 90 88 00 00 00 83 F8 FF 74 ? C9 31 C0 C3 0F 1F 44 00 00 48 8D 05 ? ? ? ? 48 8B 00 48 8B 78 10 48 85 FF 74 ? 48 8B 07 48 8B 75 F8 FF 50 28 C9 83 F8 03";
constexpr const char* party_can_invite =
  "55 48 89 E5 41 56 49 89 F6 41 55 41 54 49 89 FC 53 48 83 7F 30 00 74 ? 80 7F 40 00 75 ? 45 31 ED";
constexpr const char* party_incoming_invites_debug =
  "83 3F 01 74 ? 48 8D 3D ? ? ? ? 31 C0 E9 ? ? ? ? 0F 1F 44 00 00 55 48 89 E5 41 56 41 55 41 54 53 48 83 EC 20 E8 ? ? ? ? 4C 63 A0 ? ? ? ?";
constexpr const char* gc_connected_to_match_server =
  "40 84 F6 0F 84 ? ? ? ? 0F B6 97 CE 07 00 00 31 C0 89 D1 81 E1 F0 00 00 00";
constexpr const char* gc_force_ping_refresh =
  "55 31 C0 48 89 E5 41 55 41 54 4C 8D 65 B0 53 48 89 FB 48 8D 3D ? ? ? ? 48 83 EC 38 E8 ? ? ? ? 4C 89 E7 C6 83 ? ? ? ? 01";
constexpr const char* party_invite_player =
  "55 48 89 E5 41 55 49 89 FD 41 54 53 89 D3 48 81 EC 28 01 00 00 48 89 B5 C8 FE FF FF E8 ? ? ? ? 84 C0 74 ?";
constexpr const char* party_request_join_player =
  "55 48 89 E5 41 55 41 89 D5 41 54 49 89 F4 53 48 89 FB 48 83 EC 48 48 89 75 A8 E8 ? ? ? ? 84 C0 0F 84";

constexpr const char* mvm_stats_singleton =
  "31 C0 C3 66 66 2E 0F 1F 84 00 00 00 00 00 66 90 48 8B 05 ? ? ? ? C3";
constexpr int mvm_stats_singleton_offset = 16;
constexpr const char* mvm_add_local_player_upgrade =
  "55 48 89 E5 41 57 41 89 F7 41 56 41 89 D6 41 55 41 54 53 48 89 FB 48 83 EC 08 44 8B A7 A8 08 00 00";

constexpr const char* sdr_assert_reply_timeouts =
  "48 8D 15 ? ? ? ? BE 08 1A 00 00 48 8D 3D ? ? ? ? E8 ? ? ? ? 84 C0";
constexpr const char* sdr_assert_expecting_acks =
  "48 8D 15 ? ? ? ? BE FF 02 00 00 44 8B 45 ? 48 8D 3D ? ? ? ? 48 8B 4D ? 50 8B 45 ? FF 75 ? 50 8B 45 ? FF 75 ? 50 8B 45 ? FF 75 ? 50 31 C0 41 56 41 57 41 54 E8 ? ? ? ?";

}
#endif
