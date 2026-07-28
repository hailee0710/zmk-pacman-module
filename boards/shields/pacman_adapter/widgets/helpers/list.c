/*
 * Copyright (c) 2024
 * SPDX-License-Identifier: MIT
 *
 * Simple list utility implementation
 */

#include <string.h>
#include "list.h"

void list_init(struct list *list) {
    memset(list, 0, sizeof(*list));
    list->cursor = 0;
}

bool list_add(struct list *list, const char *text, uint8_t value, bool selectable) {
    if (list->count >= LIST_MAX_ITEMS) {
        return false;
    }
    list->items[list->count].text = (char *)text;
    list->items[list->count].value = value;
    list->items[list->count].selectable = selectable;
    list->count++;
    return true;
}

void list_move_up(struct list *list) {
    if (list->cursor > 0) {
        list->cursor--;
    }
}

void list_move_down(struct list *list) {
    if (list->cursor < list->count - 1) {
        list->cursor++;
    }
}

struct list_item *list_get_selected(struct list *list) {
    if (list->count == 0) return NULL;
    return &list->items[list->cursor];
}
