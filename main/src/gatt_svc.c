/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */
// Extended by the GAME2 Team
/* Includes */
#include "gatt_svc.h"
#include "common.h"
#include "bot.h"

/* Private function declarations */                
static int gomoku_bot_chr_access(uint16_t conn_handle, uint16_t attr_handle,
                            struct ble_gatt_access_ctxt *ctxt, void *arg);
static int search_depth_chr_access(uint16_t conn_handle, uint16_t attr_handle,
                            struct ble_gatt_access_ctxt *ctxt, void *arg);
static int winner_chr_access(uint16_t conn_handle, uint16_t attr_handle,
                            struct ble_gatt_access_ctxt *ctxt, void *arg);
static int safety_chr_access(uint16_t conn_handle, uint16_t attr_handle,
                            struct ble_gatt_access_ctxt *ctxt, void *arg);
static int reset_board_chr_access(uint16_t conn_handle, uint16_t attr_handle,
                            struct ble_gatt_access_ctxt *ctxt, void *arg);
static int autoplay_chr_access(uint16_t conn_handle, uint16_t attr_handle,
                            struct ble_gatt_access_ctxt *ctxt, void *arg);

/* Private variables */
/* Gomoku Bot service */
static const ble_uuid16_t gomoku_bot_svc_uuid = BLE_UUID16_INIT(0x181C);

static uint8_t gomoku_bot_chr_next_move = 255;
static uint16_t gomoku_bot_chr_val_handle;
static const ble_uuid16_t gomoku_bot_chr_uuid = BLE_UUID16_INIT(0x2A46);

static uint8_t gomoku_bot_search_depth = 5;
static uint16_t search_depth_chr_val_handle;
static const ble_uuid16_t search_depth_chr_uuid = BLE_UUID16_INIT(0x2A47);

static char gomoku_bot_winner = '\0';
static uint16_t winner_chr_val_handle;
static const ble_uuid16_t winner_chr_uuid = BLE_UUID16_INIT(0x2A48);

static uint16_t safety_chr_conn_handle = 0;
static bool safety_chr_conn_handle_inited = false;
static bool safety_ind_status = false;
static uint8_t safety_state = 0;
static uint16_t safety_chr_val_handle;
static const ble_uuid16_t safety_chr_uuid = BLE_UUID16_INIT(0x2A49);

static uint16_t reset_board_chr_val_handle;
static const ble_uuid16_t reset_board_chr_uuid = BLE_UUID16_INIT(0x2A4A);

static uint16_t autoplay_chr_val_handle;
static const ble_uuid16_t autoplay_chr_uuid = BLE_UUID16_INIT(0x2A4B);

Board game_board;

/* GATT services table */
static const struct ble_gatt_svc_def gatt_svr_svcs[] = {
    {.type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = &gomoku_bot_svc_uuid.u,
        .characteristics =
            (struct ble_gatt_chr_def[]){
                {/* Gomoku bot characteristic */
                 .uuid = &gomoku_bot_chr_uuid.u,
                 .access_cb = gomoku_bot_chr_access,
                 .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_WRITE,
                 .val_handle = &gomoku_bot_chr_val_handle},
                {/* Search depth characteristic */
                    .uuid = &search_depth_chr_uuid.u,
                    .access_cb = search_depth_chr_access,
                    .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_WRITE,
                    .val_handle = &search_depth_chr_val_handle},
                {/* Check winner characteristic */
                    .uuid = &winner_chr_uuid.u,
                    .access_cb = winner_chr_access,
                    .flags = BLE_GATT_CHR_F_READ,
                    .val_handle = &winner_chr_val_handle},
                {/* Safety system characteristic */
                    .uuid = &safety_chr_uuid.u,
                    .access_cb = safety_chr_access,
                    .flags = BLE_GATT_CHR_F_WRITE | BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_INDICATE,
                    .val_handle = &safety_chr_val_handle},
                {/* Reset board characteristic */
                    .uuid = &reset_board_chr_uuid.u,
                    .access_cb = reset_board_chr_access,
                    .flags = BLE_GATT_CHR_F_WRITE,
                    .val_handle = &reset_board_chr_val_handle},
                {/* Autoplay characteristic */
                    .uuid = &autoplay_chr_uuid.u,
                    .access_cb = autoplay_chr_access,
                    .flags = BLE_GATT_CHR_F_WRITE,
                    .val_handle = &autoplay_chr_val_handle},
                {
                    0, /* No more characteristics in this service. */
                }}},

    {
        0, /* No more services. */
    },
};

void restart_game_board() {
    for (int i = 0; i < BOARD_SIZE; i++) {
        game_board.black[i] = 0;
    }
    gomoku_bot_chr_next_move = 255;
    reset_bot();
    ESP_LOGI(TAG, "board reset.");
}

bool gomoku_bot_check_winner() {
    char winner = check_winner(&game_board);
    printf("%c\n", winner);
    if (winner == '\0') return false; // game keeps going
    gomoku_bot_winner = winner;
    restart_game_board();
    return true;
}

/* Private functions */
static int gomoku_bot_chr_access(uint16_t conn_handle, uint16_t attr_handle,
                                 struct ble_gatt_access_ctxt *ctxt, void *arg) {
    /* Local variables */
    int rc;

    /* Handle access events */
    switch (ctxt->op) {

    /* Read characteristic event */
    case BLE_GATT_ACCESS_OP_READ_CHR:
        /* Verify connection handle */
        if (conn_handle != BLE_HS_CONN_HANDLE_NONE) {
            ESP_LOGI(TAG, "characteristic read; conn_handle=%d attr_handle=%d",
                     conn_handle, attr_handle);
        } else {
            ESP_LOGI(TAG, "characteristic read by nimble stack; attr_handle=%d",
                     attr_handle);
        }

        /* Verify attribute handle */
        if (attr_handle == gomoku_bot_chr_val_handle) {
            /* Update access buffer value */
            rc = os_mbuf_append(ctxt->om, &gomoku_bot_chr_next_move,
                                sizeof(gomoku_bot_chr_next_move));
            ESP_LOGI(TAG, "move read: %d", gomoku_bot_chr_next_move);
            return rc == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
        }
        goto error;
    
    case BLE_GATT_ACCESS_OP_WRITE_CHR:
        /* Verify connection handle */
        if (conn_handle != BLE_HS_CONN_HANDLE_NONE) {
            ESP_LOGI(TAG, "characteristic write; conn_handle=%d attr_handle=%d",
                     conn_handle, attr_handle);
        } else {
            ESP_LOGI(TAG,
                     "characteristic write by nimble stack; attr_handle=%d",
                     attr_handle);
        }
        /* Verify attribute handle */
        if (attr_handle == gomoku_bot_chr_val_handle) {
            /* Verify access buffer length */
            if (ctxt->om->om_len == sizeof(Board)/2) {
                for (int i = 0; i < 2*BOARD_SIZE; i += 2) { // update game board with white pieces
                    game_board.white[i/2] = (ctxt->om->om_data[i] << 8) | ctxt->om->om_data[i+1];
                }
                // find next move and update game board with new black piece
                print_board(&game_board);
                
                if (gomoku_bot_check_winner()) return 0; // check if white won or draw

                gomoku_bot_chr_next_move = bot_place_piece(&game_board, BLACK, gomoku_bot_search_depth);
                ESP_LOGI(TAG, "move: %d", gomoku_bot_chr_next_move);
                place_piece(&game_board, gomoku_bot_chr_next_move % BOARD_SIZE, gomoku_bot_chr_next_move / BOARD_SIZE, BLACK);
                
                printf("The value sent from esp32 to pic18 is: %d ", gomoku_bot_chr_next_move);
                nrf_send_data(&gomoku_bot_chr_next_move, 1);
                
                gomoku_bot_check_winner(); // check if black won or draw
            } else {
                goto error;
            }
            return 0;
        }
        goto error;

    /* Unknown event */
    default:
        goto error;
    }

error:
    ESP_LOGE(
        TAG,
        "unexpected access operation to gomoku bot characteristic, opcode: %d",
        ctxt->op);
    return BLE_ATT_ERR_UNLIKELY;
}

static int search_depth_chr_access(uint16_t conn_handle, uint16_t attr_handle,
                                 struct ble_gatt_access_ctxt *ctxt, void *arg) {
    /* Local variables */
    int rc;

    /* Handle access events */
    switch (ctxt->op) {

    /* Read characteristic event */
    case BLE_GATT_ACCESS_OP_READ_CHR:
        /* Verify connection handle */
        if (conn_handle != BLE_HS_CONN_HANDLE_NONE) {
            ESP_LOGI(TAG, "characteristic read; conn_handle=%d attr_handle=%d",
                     conn_handle, attr_handle);
        } else {
            ESP_LOGI(TAG, "characteristic read by nimble stack; attr_handle=%d",
                     attr_handle);
        }

        /* Verify attribute handle */
        if (attr_handle == search_depth_chr_val_handle) {
            /* Update access buffer value */
            rc = os_mbuf_append(ctxt->om, &gomoku_bot_search_depth,
                                sizeof(gomoku_bot_search_depth));
            ESP_LOGI(TAG, "move read: %d", gomoku_bot_search_depth);
            return rc == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
        }
        goto error;
    
    case BLE_GATT_ACCESS_OP_WRITE_CHR:
        /* Verify connection handle */
        if (conn_handle != BLE_HS_CONN_HANDLE_NONE) {
            ESP_LOGI(TAG, "characteristic write; conn_handle=%d attr_handle=%d",
                     conn_handle, attr_handle);
        } else {
            ESP_LOGI(TAG,
                     "characteristic write by nimble stack; attr_handle=%d",
                     attr_handle);
        }
        /* Verify attribute handle */
        if (attr_handle == search_depth_chr_val_handle) {
            /* Verify access buffer length */
            if (ctxt->om->om_len == sizeof(uint8_t) && ctxt->om->om_data[0] < 10) { // change search depth (difficulty)
                gomoku_bot_search_depth = ctxt->om->om_data[0];
                set_do_quiescence(ctxt->om->om_data[0] > 4);
                set_better_move_order(ctxt->om->om_data[0] > 2);
                ESP_LOGI(TAG, "search depth: %d", gomoku_bot_search_depth);
            } else {
                goto error;
            }
            return 0;
        }
        goto error;

    /* Unknown event */
    default:
        goto error;
    }

error:
    ESP_LOGE(
        TAG,
        "unexpected access operation to gomoku bot characteristic, opcode: %d",
        ctxt->op);
    return BLE_ATT_ERR_UNLIKELY;
}

static int winner_chr_access(uint16_t conn_handle, uint16_t attr_handle,
                                 struct ble_gatt_access_ctxt *ctxt, void *arg) {
    /* Local variables */
    int rc;

    /* Handle access events */
    switch (ctxt->op) {

    /* Read characteristic event */
    case BLE_GATT_ACCESS_OP_READ_CHR:
        /* Verify connection handle */
        if (conn_handle != BLE_HS_CONN_HANDLE_NONE) {
            ESP_LOGI(TAG, "characteristic read; conn_handle=%d attr_handle=%d",
                     conn_handle, attr_handle);
        } else {
            ESP_LOGI(TAG, "characteristic read by nimble stack; attr_handle=%d",
                     attr_handle);
        }

        /* Verify attribute handle */
        if (attr_handle == winner_chr_val_handle) {
            /* Update access buffer value */
            rc = os_mbuf_append(ctxt->om, &gomoku_bot_winner, sizeof(gomoku_bot_winner));
            ESP_LOGI(TAG, "winner read: %d", gomoku_bot_winner);
            gomoku_bot_winner = '\0';
            return rc == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
        }
        goto error;

    /* Unknown event */
    default:
        goto error;
    }

error:
    ESP_LOGE(
        TAG,
        "unexpected access operation to winner characteristic, opcode: %d",
        ctxt->op);
    return BLE_ATT_ERR_UNLIKELY;
}

static int safety_chr_access(uint16_t conn_handle, uint16_t attr_handle,
                                 struct ble_gatt_access_ctxt *ctxt, void *arg) {
    /* Local variables */
    int rc;

    /* Handle access events */
    switch (ctxt->op) {

    /* Read characteristic event */
    case BLE_GATT_ACCESS_OP_READ_CHR:
        /* Verify connection handle */
        if (conn_handle != BLE_HS_CONN_HANDLE_NONE) {
            ESP_LOGI(TAG, "characteristic read; conn_handle=%d attr_handle=%d",
                     conn_handle, attr_handle);
        } else {
            ESP_LOGI(TAG, "characteristic read by nimble stack; attr_handle=%d",
                     attr_handle);
        }

        /* Verify attribute handle */
        if (attr_handle == safety_chr_val_handle) {
            /* Update access buffer value */
            rc = os_mbuf_append(ctxt->om, &safety_state, sizeof(safety_state));
            ESP_LOGI(TAG, "safety system read: %d", safety_state);
            return rc == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
        }
        goto error;
    
    case BLE_GATT_ACCESS_OP_WRITE_CHR:
        /* Verify connection handle */
        if (conn_handle != BLE_HS_CONN_HANDLE_NONE) {
            ESP_LOGI(TAG, "characteristic write; conn_handle=%d attr_handle=%d",
                     conn_handle, attr_handle);
        } else {
            ESP_LOGI(TAG,
                     "characteristic write by nimble stack; attr_handle=%d",
                     attr_handle);
        }
        /* Verify attribute handle */
        if (attr_handle == safety_chr_val_handle) {
            /* Verify access buffer length */
            if (ctxt->om->om_len == sizeof(uint8_t)) { // retry move after safety system interrupted it
                printf("The value sent from esp32 to pic18 is: %d ", gomoku_bot_chr_next_move);
                nrf_send_data(&gomoku_bot_chr_next_move, 1);
            } else {
                goto error;
            }
            return 0;
        }
        goto error;

    /* Unknown event */
    default:
        goto error;
    }

error:
    ESP_LOGE(
        TAG,
        "unexpected access operation to winner characteristic, opcode: %d",
        ctxt->op);
    return BLE_ATT_ERR_UNLIKELY;
}

static int reset_board_chr_access(uint16_t conn_handle, uint16_t attr_handle,
                                 struct ble_gatt_access_ctxt *ctxt, void *arg) {
    /* Handle access events */
    switch (ctxt->op) {

    case BLE_GATT_ACCESS_OP_WRITE_CHR:
        /* Verify connection handle */
        if (conn_handle != BLE_HS_CONN_HANDLE_NONE) {
            ESP_LOGI(TAG, "characteristic write; conn_handle=%d attr_handle=%d",
                     conn_handle, attr_handle);
        } else {
            ESP_LOGI(TAG,
                     "characteristic write by nimble stack; attr_handle=%d",
                     attr_handle);
        }
        /* Verify attribute handle */
        if (attr_handle == reset_board_chr_val_handle) {
            /* Verify access buffer length */
            if (ctxt->om->om_len == sizeof(uint8_t)) {
                restart_game_board();
            } else {
                goto error;
            }
            return 0;
        }
        goto error;

    /* Unknown event */
    default:
        goto error;
    }

error:
    ESP_LOGE(
        TAG,
        "unexpected access operation to gomoku bot characteristic, opcode: %d",
        ctxt->op);
    return BLE_ATT_ERR_UNLIKELY;
}

static int autoplay_chr_access(uint16_t conn_handle, uint16_t attr_handle,
                                 struct ble_gatt_access_ctxt *ctxt, void *arg) {
    /* Handle access events */
    switch (ctxt->op) {

    case BLE_GATT_ACCESS_OP_WRITE_CHR:
        /* Verify connection handle */
        if (conn_handle != BLE_HS_CONN_HANDLE_NONE) {
            ESP_LOGI(TAG, "characteristic write; conn_handle=%d attr_handle=%d",
                     conn_handle, attr_handle);
        } else {
            ESP_LOGI(TAG,
                     "characteristic write by nimble stack; attr_handle=%d",
                     attr_handle);
        }
        /* Verify attribute handle */
        if (attr_handle == autoplay_chr_val_handle) {
            /* Verify access buffer length */
            if (ctxt->om->om_len == sizeof(char)) {
                uint8_t next_move = bot_place_piece(&game_board, ctxt->om->om_data[0], gomoku_bot_search_depth);
                ESP_LOGI(TAG, "move: %d", next_move);
                place_piece(&game_board, next_move % BOARD_SIZE, next_move / BOARD_SIZE, ctxt->om->om_data[0]);

                print_board(&game_board);
                
                printf("The value sent from esp32 to pic18 is: %d ", next_move);
                nrf_send_data(&next_move, 1);
                
                gomoku_bot_check_winner(); // check if won or draw
            } else {
                goto error;
            }
            return 0;
        }
        goto error;

    /* Unknown event */
    default:
        goto error;
    }

error:
    ESP_LOGE(
        TAG,
        "unexpected access operation to gomoku bot characteristic, opcode: %d",
        ctxt->op);
    return BLE_ATT_ERR_UNLIKELY;
}

void send_safety_system_indication(uint8_t trigger) {
    safety_state = trigger;
    if (safety_ind_status && safety_chr_conn_handle_inited) {
        ble_gatts_indicate(safety_chr_conn_handle,
                        safety_chr_val_handle);
        ESP_LOGI(TAG, "safety system notification sent!");
    }
}

void gatt_svr_subscribe_cb(struct ble_gap_event *event) {
    /* Check connection handle */
    if (event->subscribe.conn_handle != BLE_HS_CONN_HANDLE_NONE) {
        ESP_LOGI(TAG, "subscribe event; conn_handle=%d attr_handle=%d",
                event->subscribe.conn_handle, event->subscribe.attr_handle);
    } else {
        ESP_LOGI(TAG, "subscribe by nimble stack; attr_handle=%d",
                event->subscribe.attr_handle);
    }

    /* Check attribute handle */
    if (event->subscribe.attr_handle == safety_chr_val_handle) {
        /* Update heart rate subscription status */
        safety_chr_conn_handle = event->subscribe.conn_handle;
        safety_chr_conn_handle_inited = true;
        safety_ind_status = event->subscribe.cur_indicate;
    }
}

/*
 *  Handle GATT attribute register events
 *      - Service register event
 *      - Characteristic register event
 *      - Descriptor register event
 */
void gatt_svr_register_cb(struct ble_gatt_register_ctxt *ctxt, void *arg) {
    /* Local variables */
    char buf[BLE_UUID_STR_LEN];

    /* Handle GATT attributes register events */
    switch (ctxt->op) {

    /* Service register event */
    case BLE_GATT_REGISTER_OP_SVC:
        ESP_LOGD(TAG, "registered service %s with handle=%d",
                 ble_uuid_to_str(ctxt->svc.svc_def->uuid, buf),
                 ctxt->svc.handle);
        break;

    /* Characteristic register event */
    case BLE_GATT_REGISTER_OP_CHR:
        ESP_LOGD(TAG,
                 "registering characteristic %s with "
                 "def_handle=%d val_handle=%d",
                 ble_uuid_to_str(ctxt->chr.chr_def->uuid, buf),
                 ctxt->chr.def_handle, ctxt->chr.val_handle);
        break;

    /* Descriptor register event */
    case BLE_GATT_REGISTER_OP_DSC:
        ESP_LOGD(TAG, "registering descriptor %s with handle=%d",
                 ble_uuid_to_str(ctxt->dsc.dsc_def->uuid, buf),
                 ctxt->dsc.handle);
        break;

    /* Unknown event */
    default:
        assert(0);
        break;
    }
}

/*
 *  GATT server initialization
 *      1. Initialize GATT service
 *      2. Update NimBLE host GATT services counter
 *      3. Add GATT services to server
 */
int gatt_svc_init(void) {
    /* Local variables */
    int rc;

    /* 1. GATT service initialization */
    ble_svc_gatt_init();

    /* 2. Update GATT services counter */
    rc = ble_gatts_count_cfg(gatt_svr_svcs);
    if (rc != 0) {
        return rc;
    }

    /* 3. Add GATT services */
    rc = ble_gatts_add_svcs(gatt_svr_svcs);
    if (rc != 0) {
        return rc;
    }

    return 0;
}
