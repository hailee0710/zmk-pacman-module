/*
 * Copyright (c) 2024
 * SPDX-License-Identifier: MIT
 *
 * Simple list utility
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>

#define LIST_MAX_ITEMS 20

struct list_item {
    char *text;
    uint8_t value;
    bool selectable;
};

struct list {
    struct list_item items[LIST_MAX_ITEMS];
    uint8_t count;
    uint8_t cursor;
};

void list_init(struct list *list);
bool list_add(struct list *list, const char *text, uint8_t value, bool selectable);
void list_move_up(struct list *list);
void list_move_down(struct list *list);
struct list_item *list_get_selected(struct list *list);
