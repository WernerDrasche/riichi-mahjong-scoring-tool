#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

char input[50];

#define NUM_YAKU 41
enum YAKU {
    MAHJONG_FIRST_ROUND,
    FOUR_KONGS,
    NINE_GATES,
    ALL_GREEN,
    ALL_TERMINALS,
    ALL_HONORS,
    BIG_THREE_DRAGONS,
    BIG_FOUR_WINDS,
    LITTLE_FOUR_WINDS,
    FOUR_CONCEILED_PUNGS,
    THIRTEEN_ORPHANS,
    FULL_FLUSH,
    LITTLE_THREE_DRAGONS,
    ONLY_TERMINALS_AND_HONORS,
    TWICE_PURE_DOUBLE_CHOW,
    TERMINAL_IN_ALL_SETS,
    HALF_FLUSH,
    SEVEN_PAIRS,
    THREE_KONGS,
    TRIPLE_PUNG,
    THREE_CONCEILED_PUNGS,
    ALL_PUNGS,
    PURE_STRAIGHT,
    TRIPLE_CHOW,
    OUTSIDE_HAND,
    ALL_SIMPLES,
    PUNG_OF_GREEN_DRAGON,
    PUNG_OF_RED_DRAGON,
    PUNG_OF_WHITE_DRAGON,
    PUNG_OF_SEAT_WIND,
    PUNG_OF_ROUND_WIND,
    PURE_DOUBLE_CHOW,
    PINFU,
    FULLY_CONCEILED_HAND,
    RIICHI,
    DOUBLE_RIICHI,
    ONE_SHOT,
    LAST_DRAW,
    LAST_DISCARD,
    DEAD_WALL,
    ROBBING_THE_KONG,
};

const unsigned char han_closed[] = {13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 6, 2, 2, 3, 3, 3, 2, 2, 2, 2, 2, 2, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1};
const unsigned char han_open[] =   {0,  13, 0,  13, 13, 13, 13, 13, 13, 0,  0,  5, 2, 2, 0, 2, 2, 0, 2, 2, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1};

const char *yaku_names[] = {
    "Mahjong on first round",
    "Four Kongs",
    "Nine Gates",
    "All Green",
    "All Terminals",
    "All Honors",
    "Big Three Dragons",
    "Big Four Winds",
    "Little Four Winds",
    "Four Conceiled Pungs",
    "Thirteen Orphans",
    "Full Flush",
    "Little Three Dragons",
    "Only Terminals and Honors",
    "Twice Pure Double Chow",
    "Terminal in all Sets",
    "Half Flush",
    "Seven Pairs",
    "Three Kongs",
    "Triple Pung",
    "Three Conceiled Pungs",
    "All Pungs",
    "Pure Straight",
    "Triple Chow",
    "Outside Hand",
    "All Simples",
    "Pung of Green Dragon",
    "Pung of Red Dragon",
    "Pung of White Dragon",
    "Pung of Seat Wind",
    "Pung of Round Wind",
    "Pure Double Chow",
    "Pinfu",
    "Fully Conceiled Hand",
    "Riichi",
    "Double Riichi",
    "One Shot",
    "Last Draw from the Wall",
    "Last Discard",
    "Dead Wall Draw",
    "Robbing the Kong",
};

enum GROUP {TRIPLE, QUAD, SEQUENCE, PAIR};

enum HONOR {
    DRAGON = 10,
    EAST = 11,
    SOUTH = 12,
    WEST = 13,
    NORTH = 14,
};

enum COLOR {RED, GREEN, BLUE};

struct tile {
    enum COLOR color;
    unsigned char value;
};

struct group {
    enum GROUP type;
    struct tile start;
    bool open;
};

enum WAIT {OPEN_WAIT, CLOSED_WAIT, EDGE_WAIT, PAIR_WAIT};

#define MAX_HAND_SIZE 7
struct score_info {
    unsigned char winner, from, round_wind;
    struct tile winning_tile;
    struct group hand[MAX_HAND_SIZE];
    unsigned char hand_size;
    enum WAIT wait;
    bool open_hand;
    bool tsumo;
    bool draw;
    bool tenpai[4];
    bool yaku[NUM_YAKU];
    unsigned char han, dora, fu;
    unsigned char riichi_sticks, honba_counter;
    int delta[4];
};

struct state {
    unsigned char num_players, round_wind, round, dealer;
    unsigned char riichi_sticks, honba_counter;
    char *players[4];
    int points[4];
};

struct state_history {
    struct state *states;
    unsigned char len;
    unsigned char undo_depth;
    unsigned char push_depth;
    char idx;
    bool modified;
};

void init_state_history(struct state_history *history, unsigned char len) {
    struct state *states = malloc(len * sizeof(struct state));
    *history = (struct state_history){
        .states = states,
        .len = len,
        .idx = -1,
    };
}

void deinit_state_history(struct state_history *history) {
    free(history->states);
}

void push_state(struct state_history *history, struct state *state) {
    char idx = history->idx;
    idx = (idx + 1) % history->len;
    history->states[idx] = *state;
    history->idx = idx;
    history->undo_depth = 0;
    if (history->push_depth < history->len)
        ++history->push_depth;
}

int undo(struct state_history *history, struct state *state) {
    if (!history->push_depth) {
        printf("Error: nothing to undo\n");
        return - 1;
    }
    char idx = history->idx - 1;
    if (idx < 0)
        idx += history->len;
    history->idx = idx;
    *state = history->states[idx];
    history->modified = true;
    ++history->undo_depth;
    --history->push_depth;
    return 0;
}

int redo(struct state_history *history, struct state *state) {
    if (!history->undo_depth) {
        printf("Error: nothing to redo\n");
        return -1;
    }
    char idx = (history->idx + 1) % history->len;
    history->idx = idx;
    *state = history->states[idx];
    history->modified = true;
    ++history->push_depth;
    --history->undo_depth;
    return 0;
}

#define is_wind(V) (EAST <= V && V <= NORTH)
#define is_honor(V) (is_wind(V) || V == DRAGON)
#define is_terminal(V) (V == 1 || V == 9)
#define is_triple_or_quad(G) (G.type <= QUAD)

//expects a valid group
void print_group(const struct group group) {
    char c;
    switch (group.start.value) {
        case DRAGON: c = 'd'; break;
        case EAST:   c = 'e'; break;
        case SOUTH:  c = 's'; break;
        case WEST:   c = 'w'; break;
        case NORTH:  c = 'n'; break;
        default:     c = group.start.value + 48;
    }
    if (!is_wind(group.start.value)) {
        switch (group.start.color) {
            case RED:   printf("\x1b[31m"); break;
            case GREEN: printf("\x1b[32m"); break;
            case BLUE:  printf("\x1b[34m"); break;
        }
    }
    if (group.open) printf("\x1b[2;3m");
    else printf("\x1b[1m");
    switch (group.type) {
        case QUAD:     putchar(c);
        case TRIPLE:   putchar(c);
        case PAIR:     putchar(c); putchar(c); break;
        case SEQUENCE: putchar(c); putchar(c + 1); putchar(c + 2);
    }
    printf("\x1b[0m");
}

void print_hand(const struct score_info *score_info) {
    for (int i = 0; i < score_info->hand_size; ++i) {
        struct group group = score_info->hand[i];
        print_group(group);
        putchar(' ');
    }
    putchar('\n');
}

void error_expected(const char *expected, char got) {
    printf("Error: expected %s, got %c\n", expected, got);
}

int read_line_to_input(void) {
    char *ret = fgets(input, sizeof(input), stdin);
    if (input != ret) return -1;
    if (strncmp(input, "quit", 4) == 0) return -1;
    return 0;
}

int get_number(const char *msg) {
    while (true) {
        printf("%s", msg);
        if (read_line_to_input() < 0) return -1;
        int number = atoi(input);
        if (number < 0) continue;
        return number;
    }
}

unsigned char char_to_direction(char c) {
    switch (c) {
        case 'e': 
        case 'E': return EAST;
        case 's':
        case 'S': return SOUTH;
        case 'w':
        case 'W': return WEST;
        case 'n':
        case 'N': return NORTH;
        default: return 0;
    }
}

enum ROUND_SUMMARY {WINNER, TSUMO, RON_FROM, BONUS, DRAW, YAKUMAN};

int round_summary(struct score_info *score_info, struct state *game_state, struct state_history *history) {
retry:
    printf("Round summary: ");
    if (read_line_to_input() < 0) return -1;
    //TODO
    if (!strncmp(input, "undo", 4)) {
        if (undo(history, game_state) < 0) goto retry;
        return 0;
    } else if (!strncmp(input, "redo", 4)) {
        if (redo(history, game_state) < 0) goto retry;
        return 0;
    } else if (!strncmp(input, "save", 4)) {

    } else if (!strncmp(input, "load", 4)) {

    }
    bool limit_hand = false;
    struct score_info info = {0};
    enum ROUND_SUMMARY state = WINNER;
    for (int i = 0; i < sizeof(input); ++i) {
        char c = input[i];
        if (c == ' ') continue;
        if (c == '\n' || c == 0) {
            if (state >= BONUS) break;
            printf("Error: premature end-of-line\n");
            goto retry;
        }
        switch (state) {
            case WINNER:
                if (c == 'd' || c == 'D') {
                    info.draw = true;
                    state = DRAW;
                    continue;
                }
                info.winner = char_to_direction(c);
                if (!info.winner) {
                    error_expected("e|s|w|n", c);
                    goto retry;
                }
                if (info.winner == NORTH && game_state->num_players == 3) {
                    printf("Error: there is no north seat in a three-player game\n");
                    goto retry;
                }
                state = TSUMO;
                continue;
            case DRAW:
                if ('1' <= c && c <= '4') {
                    info.riichi_sticks = c - 48;
                    continue;
                }
                unsigned char direction = char_to_direction(c);
                if (!direction) {
                    error_expected("e|s|w|n|1..4", c);
                    goto retry;
                }
                if (direction == NORTH && game_state->num_players == 3) {
                    printf("Error: there is no north seat in a three-player game\n");
                    goto retry;
                }
                info.tenpai[direction - EAST] = true;
                continue;
            case TSUMO:
                switch (c) {
                    case 'r':
                    case 'R':
                        state = RON_FROM;
                        continue;
                    case 't':
                    case 'T':
                        info.tsumo = true;
                        state = BONUS;
                        continue;
                    default:
                        error_expected("r|t", c);
                        goto retry;
                }
            case RON_FROM:
                info.from = char_to_direction(c);
                if (!info.winner) {
                    error_expected("e|s|w|n", c);
                    goto retry;
                }
                if (info.from == info.winner) {
                    printf("Error: can't ron from winner\n");
                    goto retry;
                }
                if (info.from == NORTH && game_state->num_players == 3) {
                    printf("Error: there is no north seat in a three-player game\n");
                    goto retry;
                }
                state = BONUS;
                continue;
            case BONUS:
                switch (c) {
                    case 'r':
                    case 'R':
                        if (!info.yaku[DOUBLE_RIICHI]) {
                            if (info.yaku[RIICHI]) {
                                info.yaku[DOUBLE_RIICHI] = true;
                                info.yaku[RIICHI] = false;
                            } else {
                                info.yaku[RIICHI] = true;
                            }
                        }
                        continue;
                    case 'o':
                    case 'O':
                        info.yaku[ONE_SHOT] = true;
                        continue;
                    case 'l':
                    case 'L':
                        if (info.tsumo)
                            info.yaku[LAST_DRAW] = true;
                        else
                            info.yaku[LAST_DISCARD] = true;
                        continue;
                    case 'w':
                    case 'W':
                        info.yaku[DEAD_WALL] = true;
                        continue;
                    case 'k':
                    case 'K':
                        info.yaku[ROBBING_THE_KONG] = true;
                        continue;
                    case 'y':
                    case 'Y':
                        state = YAKUMAN;
                        continue;
                    case '1':
                    case '2':
                    case '3':
                        info.riichi_sticks = c - 48;
                        continue;
                    default:
                        error_expected("r|y", c);
                        goto retry;
                }
            case YAKUMAN:
                switch (c) {
                    case 'o':
                    case 'O':
                        info.yaku[THIRTEEN_ORPHANS] = true;
                        limit_hand = true;
                        continue;
                    case '9':
                        info.yaku[NINE_GATES] = true;
                        limit_hand = true;
                        continue;
                    case 'm':
                    case 'M':
                        info.yaku[MAHJONG_FIRST_ROUND] = true;
                        limit_hand = true;
                        continue;
                    default:
                        error_expected("o", c);
                        goto retry;
                }
        }
    }
    *score_info = info;
    return limit_hand;
}

int get_hand(struct score_info *score_info) {
    struct group current;
    struct group hand[MAX_HAND_SIZE];
    struct tile winning_tile;
    bool open_hand;
    bool color, type, single;
    unsigned char value;
    int group_idx;
    int tile_count;
retry:
    printf("Hand: ");
    color = single = false;
    value = group_idx = tile_count = 0;
    current.open = false;
    open_hand = false;
    type = true;
    current.type = SEQUENCE;
    if (read_line_to_input() < 0) return -1;
    for (int i = 0; i < sizeof(input); ++i) {
        char c = input[i];
        if (!c) break;
        switch (c) {
            case 'r':
            case 'R':
                color = true;
                current.start.color = RED;
                continue;
            case 'g':
            case 'G':
                color = true;
                current.start.color = GREEN;
                continue;
            case 'b':
            case 'B':
                color = true;
                current.start.color = BLUE;
                continue;
            case 'p':
            case 'P':
                current.type = PAIR;
                continue;
            case 't':
            case 'T':
                current.type = TRIPLE;
                continue;
            case 'q':
            case 'Q':
                current.type = QUAD;
                continue;
            case 'm':
            case 'M':
                type = false;
                continue;
            case 'o':
            case 'O':
                current.open = true;
                open_hand = true;
                continue;
            case ' ':
            case '\n':
                if ((is_wind(value) || color) && value) {
                    if (type) {
                        if (group_idx >= MAX_HAND_SIZE) {
                            printf("Error: too many groups\n");
                            goto retry;
                        }
                        if (current.start.value > 7 && current.type == SEQUENCE) {
                            printf("Error: can't have sequence starting with either honor or n > 7\n");
                            goto retry;
                        } else if (current.open && current.type == PAIR) {
                            printf("Error: pair can't be open\n");
                            goto retry;
                        }
                        hand[group_idx++] = current;
                        if (current.type == PAIR) tile_count += 2;
                        else tile_count += 3;
                    } else if (!single) {
                        single = true;
                        winning_tile = current.start;
                    } else {
                        printf("Error: can only have one winning tile\n");
                        goto retry;
                    }
                    color = false;
                    current.open = false;
                    current.type = SEQUENCE;
                    type = true;
                    value = 0;
                }
                continue;
            default:
                value = char_to_direction(c);
                if (!value) {
                    if (c == 'd' || c == 'D')
                        value = DRAGON;
                    else if ('1' <= c && c <= '9')
                        value = c - 48;
                    else {
                        printf("Error: invalid specifier %i\n", c);
                        goto retry;
                    }
                } 
                current.start.value = value;
                continue;
        }
    }
    if (group_idx < 4) {
        //printf("group_idx = %i\n", group_idx);
        printf("Error: too few groups\n");
        goto retry;
    }
    if (!single) {
        printf("Error: need to specify winning tile\n");
        goto retry;
    }
    if (tile_count != 14) {
        printf("Error: hand needs to consist of 14 tiles\n");
        goto retry;
    }
    uint8_t count[3][18] = {0};
    for (int i = 0; i < group_idx; ++i) {
        current = hand[i];
        if (is_wind(current.start.value))
            current.start.color = RED;
        uint8_t *ptr = &count[current.start.color][current.start.value];
        //int32_t tmp4 = *(int32_t *)ptr;
        switch (current.type) {
            case QUAD:
                ++(*ptr);
            case TRIPLE:
                ++(*ptr);
            case PAIR: 
                *ptr += 2;
                break;
            case SEQUENCE:
                *(uint32_t *)ptr += 0x00010101;       
        }
        //print_group(current);
        //putchar('\n');
        //int32_t tmp0 = *(int32_t *)ptr;
        //int32_t tmp1 = ~tmp0;
        //int32_t tmp2 = tmp1 + 0x04040405;
        //int32_t tmp3 = tmp2 & 0x80808080;
        //printf("%x, %x, %x, %x, %x\n", tmp4, tmp0, tmp1, tmp2, tmp3);
        if (((~*(int32_t *)ptr) + 0x04040405) & 0x80808080) {
            printf("Error: hand contains more than 4 of a kind\n");
            goto retry;
        }
        //TODO: this doesn't work
        score_info->hand[i] = current;
    }
    score_info->winning_tile = winning_tile;
    score_info->hand_size = group_idx;
    score_info->open_hand = open_hand;
    return 0;
}

int determine_wait(struct score_info *score_info) {
    enum WAIT wait;
    bool found = false;
    int current;
    struct tile tile = score_info->winning_tile;
    for (int i = 0; i < score_info->hand_size; ++i) {
        struct group group = score_info->hand[i];
        if (group.type == SEQUENCE) {
            if (tile.value - group.start.value < 3 && tile.color == group.start.color) {
                found = true;
                current = i;
                if (tile.value == group.start.value + 1) {
                    wait = CLOSED_WAIT;
                    break;
                } else if ((tile.value == 3 && group.start.value == 1) || (tile.value == 7 && group.start.value == 7)) {
                    wait = EDGE_WAIT;
                    break;
                } else wait = OPEN_WAIT;
            }
        } else if (group.type == PAIR) {
            if (tile.value == group.start.value && (is_wind(tile.value) || tile.color == group.start.color)) {
                found = true;
                current = i;
                wait = PAIR_WAIT;
                break;
            }
        } else {
            if (tile.value == group.start.value && (is_wind(tile.value) || tile.color == group.start.color)) {
                found = true;
                current = i;
                wait = OPEN_WAIT;
                break;
            }
        }
    }
    if (!found) {
        printf("Error: winning tile is not part of hand\n");
        return -1;
    }
    score_info->hand[current].open = true;
    score_info->wait = wait;
    return 0;
}

unsigned char check_same_all_colors(struct group group, const struct score_info *score_info) {
    unsigned char result = 0;
    for (int i = 0; i < score_info->hand_size; ++i) {
        struct group cmp = score_info->hand[i];
        if ((cmp.start.value == group.start.value) &&
                (group.type == cmp.type || (is_triple_or_quad(group) &&
                                            is_triple_or_quad(cmp)))) 
            result |= 1 << cmp.start.color;
    }
    return result;
}

unsigned char find_zero_bit(unsigned char colors, unsigned char bits) {
    for (unsigned char i = 0; i < bits; ++i) {
        if (!(colors & 1)) return i;
        colors >>= 1;
    }
    printf("Internal Error: no zero bit");
    return 0;
}

int search_for_group(struct group group, const struct score_info *score_info, int exclude_lower) {
    for (int i = 0; i < score_info->hand_size; ++i) {
        if (i <= exclude_lower) continue;
        struct group cmp = score_info->hand[i];
        if (group.type == cmp.type &&
                group.start.value == cmp.start.value &&
                (is_wind(group.start.value) ||
                 group.start.color == cmp.start.color)) {
            return i;
        }
    }
    return 0;
}

void check_values(struct score_info *score_info) {
    bool all_terminals, all_terminals_honors, terminal_each, outside, all_simples;
    all_terminals = all_terminals_honors = terminal_each = outside = all_simples = true;
    for (int i = 0; i < score_info->hand_size; ++i) {
        struct group group = score_info->hand[i];
        if (group.type == SEQUENCE) {
            all_terminals = all_terminals_honors = false;
            if (group.start.value == 1 || group.start.value == 7)
                all_simples = false;
            else
                outside = false;
        } else {
            if (is_honor(group.start.value))
                all_terminals = terminal_each = all_simples = false;
            else if (is_terminal(group.start.value))
                all_simples = false;
            else
                all_terminals = all_terminals_honors = terminal_each = outside = false;
        }
    }
    if (all_terminals)
        score_info->yaku[ALL_TERMINALS] = true;
    else if (all_terminals_honors)
        score_info->yaku[ONLY_TERMINALS_AND_HONORS] = true;
    else if (terminal_each)
        score_info->yaku[TERMINAL_IN_ALL_SETS] = true;
    else if (outside)
        score_info->yaku[OUTSIDE_HAND] = true;
    else if (all_simples)
        score_info->yaku[ALL_SIMPLES] = true;
}

void check_colors(struct score_info *score_info) {
    bool all_green, all_honors, full_flush, half_flush;
    all_green = all_honors = full_flush = half_flush = true;
    enum COLOR color = score_info->hand[0].start.color;
    for (int i = 0; i < score_info->hand_size; ++i) {
        struct group group = score_info->hand[i];
        if (is_honor(group.start.value)) {
            full_flush = false;
            if (is_wind(group.start.value))
                all_green = false;
            continue;
        }
        all_honors = false;
        if (group.start.color != color) {
            all_green = full_flush = half_flush = false;
            break;
        }
        if (color != GREEN) continue;
        if (group.type == SEQUENCE && group.start.value != 2) 
            all_green = false;
        if (group.type != SEQUENCE) {
            switch (group.start.value) {
                case 2:
                case 3:
                case 4:
                case 6:
                case 8: continue;
                default: all_green = false;
            }
        }
    }
    if (all_green && half_flush && color == GREEN)
        score_info->yaku[ALL_GREEN] = true;
    else if (all_honors)
        score_info->yaku[ALL_HONORS] = true;
    else if (full_flush)
        score_info->yaku[FULL_FLUSH] = true;
    else if (half_flush)
        score_info->yaku[HALF_FLUSH] = true;
}

void check_melds(struct score_info *score_info) {
    int triples, quads, conceiled;
    triples = quads = conceiled = 0;
    unsigned char winds = 0;
    unsigned char colors;
    bool little_three, triple_pung;
    little_three = triple_pung = false;
    bool valueless_pair = true;
    for (int i = 0; i < score_info->hand_size; ++i) {
        struct group group = score_info->hand[i];
        if (is_triple_or_quad(group)) {
            if (group.type == TRIPLE) ++triples;
            else if (group.type == QUAD) ++quads;
            if (!group.open) ++conceiled;
            if (!is_wind(group.start.value)) {
                if (!triple_pung) {
                    colors = check_same_all_colors(group, score_info);
                    if (colors == 0b111) {
                        triple_pung = true;
                        if (group.start.value == DRAGON) {
                            score_info->yaku[BIG_THREE_DRAGONS] = true;
                            little_three = true;
                        } else
                            score_info->yaku[TRIPLE_PUNG] = true;
                    }
                }
                if (group.start.value == DRAGON) {
                    if (!little_three && (colors & (colors - 1))) {
                        struct tile missing_dragon = {.value = DRAGON, .color = find_zero_bit(colors, 3)};
                        //printf("DEBUG: missing_color = %i\n", find_zero_bit(colors, 3));
                        struct group pair = {.type = PAIR, .start = missing_dragon};
                        little_three = true;
                        score_info->yaku[LITTLE_THREE_DRAGONS] = (bool)search_for_group(pair, score_info, -1);
                    }
                    if (group.start.color == GREEN)
                        score_info->yaku[PUNG_OF_GREEN_DRAGON] = true;
                    else if (group.start.color == RED)
                        score_info->yaku[PUNG_OF_RED_DRAGON] = true;
                    else if (group.start.color == BLUE)
                        score_info->yaku[PUNG_OF_WHITE_DRAGON] = true;
                }
            } else {
                unsigned char wind = group.start.value;
                winds |= 1 << (wind - EAST);
                if (wind == score_info->winner)
                    score_info->yaku[PUNG_OF_SEAT_WIND] = true;
                if (wind == score_info->round_wind)
                    score_info->yaku[PUNG_OF_ROUND_WIND] = true;
            }
        } else if (group.type == PAIR) {
            if (group.start.value == score_info->winner ||
                    group.start.value == score_info->round_wind ||
                    group.start.value == DRAGON) {
                valueless_pair = false;
            }
        }
    }
    if (winds == 0b1111)
        score_info->yaku[BIG_FOUR_WINDS] = true;
    else if (winds & (winds - 1)) {
        struct tile missing_wind = {.value = find_zero_bit(winds, 4)};
        struct group pair = {.type = PAIR, .start = missing_wind};
        score_info->yaku[LITTLE_FOUR_WINDS] = (bool)search_for_group(pair, score_info, -1);
    }
    if (triples + quads == 4)
        score_info->yaku[ALL_PUNGS] = true;
    if (conceiled == 4)
        score_info->yaku[FOUR_CONCEILED_PUNGS] = true;
    else if (conceiled == 3)
        score_info->yaku[THREE_CONCEILED_PUNGS] = true;
    if (quads == 4)
        score_info->yaku[FOUR_KONGS] = true;
    else if (quads == 3)
        score_info->yaku[THREE_KONGS] = true;

    if (triples + quads > 2) return;
    bool tpdc, pdc, triple_chow;
    tpdc = pdc = triple_chow = false;
    int other = 0;
    for (int i = 0; i < score_info->hand_size; ++i) {
        struct group group = score_info->hand[i];
        if (group.type != SEQUENCE) continue;
        if (!triple_chow && check_same_all_colors(group, score_info) == 0b111)
            triple_chow = true;
        if (i != other || i == 0) {
            int other_before = other;
            other = search_for_group(group, score_info, i);
            if (other) {
                if (other_before)
                    tpdc = true;
                pdc = true;
            }
        }
        if (group.start.value == 1) {
            group.start.value = 4;
            if (!search_for_group(group, score_info, -1)) continue;
            group.start.value = 7;
            if (search_for_group(group, score_info, -1)) {
                score_info->yaku[PURE_STRAIGHT] = true;
                break;
            }
        }
    }
    if (tpdc)
        score_info->yaku[TWICE_PURE_DOUBLE_CHOW] = true;
    else if (pdc)
        score_info->yaku[PURE_DOUBLE_CHOW] = true;
    if (triple_chow)
        score_info->yaku[TRIPLE_CHOW] = true;
    if (triples + quads == 0 && !score_info->open_hand && score_info->wait == OPEN_WAIT && valueless_pair)
        score_info->yaku[PINFU] = true;
}

int check_yaku(struct score_info *score_info) {
    if (score_info->hand_size == 7) {
        score_info->yaku[SEVEN_PAIRS] = true;
    } 
    check_melds(score_info);
    check_colors(score_info);
    check_values(score_info);
    if (score_info->tsumo && !score_info->open_hand)
        score_info->yaku[FULLY_CONCEILED_HAND] = true;
    return 0;
}

unsigned int round_to_next(unsigned int next, unsigned int val) {
    unsigned int result = val + next - 1;
    result /= next;
    result *= next;
    return result;
}

unsigned char calculate_fu_for_group(struct group group, const struct score_info *score_info) {
    unsigned char fu = 0;
    bool flag = false;
    if (group.type == SEQUENCE) return 0;
    if (group.type == PAIR) {
        if (group.start.value == score_info->winner) {
            flag = true;
            printf("%-27s", "Pair of Seat Wind");
            print_group(group);
            printf("%5c%-2u Fu\n", ' ', 2);
        }
        if (group.start.value == score_info->round_wind) {
            fu = 2;
            printf("%-27s", "Pair of Round Wind");
        } else if (group.start.value == DRAGON) {
            fu = 2;
            printf("%-27s", "Pair of Dragons");
        }
        if (!fu) return 0;
        print_group(group);
        printf("%5c%-2u Fu\n", ' ', fu);
    } else if (group.type == TRIPLE) {
        fu = group.open ? 2 : 4;
        if (is_terminal(group.start.value) || is_honor(group.start.value)) {
            fu *= 2;
            printf("%-27s", "Pung of Terminals/Honors");
        } else {
            printf("%-27s", "Pung of Simples");
        }
        print_group(group);
        printf("%4c%-2u Fu\n", ' ', fu);
    } else if (group.type == QUAD) {
        fu = group.open ? 8 : 16;
        if (is_terminal(group.start.value) || is_honor(group.start.value)) {
            fu *= 2;
            printf("%-27s", "Kong of Terminals/Honors");
        } else {
            printf("%-27s", "Kong of Simples");
        }
        print_group(group);
        printf("%3c%-2u Fu\n", ' ', fu);
    }
    if (flag) fu += 2;
    return fu;
}

void calculate_fu(struct score_info *score_info) {
    unsigned char fu, wait, tsumo;
    if (score_info->yaku[SEVEN_PAIRS]) {
        fu = 25;
        printf("%-34s%-2u Fu\n", "Seven Pairs", fu);
    } else if (score_info->yaku[PINFU]) {
        if (score_info->tsumo) {
            fu = 20;
            printf("%-34s%-2u Fu\n", "Pinfu Tsumo", fu);
        } else {
            fu = 30;
            printf("%-34s%-2u Fu\n", "Pinfu Ron", fu);
        }
    } else {
        if (!(score_info->tsumo || score_info->open_hand)) {
            fu = 30;
            printf("%-34s%-2u Fu\n", "Conceiled Hand Ron", fu);
        } else {
            fu = 20;
            printf("%-34s%-2u Fu\n", "Open Hand or Tsumo", fu);
        }
        for (int i = 0; i < score_info->hand_size; ++i) 
            fu += calculate_fu_for_group(score_info->hand[i], score_info);
        wait = score_info->wait == OPEN_WAIT ? 0 : 2;
        switch (score_info->wait) {
            case EDGE_WAIT:
                printf("%-34s%-2u Fu\n", "Edge Wait", wait);
                break;
            case CLOSED_WAIT:
                printf("%-34s%-2u Fu\n", "Closed Wait", wait);
                break;
            case PAIR_WAIT:
                printf("%-34s%-2u Fu\n", "Pair Wait", wait);
                break;
            default: break;
        }
        tsumo = 0;
        if (score_info->tsumo) {
            tsumo = 2;
            printf("%-34s%-2u Fu\n", "Self Draw", tsumo);
        }
        fu += wait + tsumo;
        if (fu == 20 && score_info->open_hand) {
            unsigned char open_pinfu = 10;
            printf("%-34s%-2u Fu\n", "Open Pinfu", open_pinfu);
            fu += open_pinfu;
        }
        fu = round_to_next(10, fu);
    }
    printf("%34c%-2u Fu\n", ' ', fu);
    score_info->fu = fu;
}

void print_scores(struct score_info *score_info, struct state *state, bool ignore_delta) {
    for (int i = 0; i < state->num_players; ++i) {
        char *name = state->players[i];
        unsigned char direction = (i - state->round + state->num_players + 1) % state->num_players;
        char c;
        switch (direction) {
            case 0:  c = 'E'; break;
            case 1: c = 'S'; break;
            case 2:  c = 'W'; break;
            case 3: c = 'N'; break;
        }
        snprintf(input, sizeof(input), "%s (%c)", name, c);
        int before = state->points[i];
        int after = before + score_info->delta[direction];
        if (ignore_delta) {
            printf("%-17s  %-6i\n", input, before);
            continue;
        } else if (after == before)
            printf("%-17s  %-6i  %6c  %-6i\n", input, before, ' ', after);
        else
            printf("%-17s  %-6i  %+-6i  %-6i\n", input, before, score_info->delta[direction], after);
        state->points[i] = after;
    }
}

int calculate_score(struct score_info *score_info, struct state *state) {
    int score = 0;
    const unsigned char *han_values = score_info->open_hand ? han_open : han_closed;
    unsigned char han, dora, fu;
    han = 0;
    dora = score_info->dora;
    fu = score_info->fu;
    for (int i = 0; i < NUM_YAKU; ++i) {
        if (score_info->yaku[i]) {
            unsigned char value = han_values[i];
            if (value) {
                printf("%-34s%-2u Han\n", yaku_names[i], value);
                han += value;
            }
        }
        if (i == THIRTEEN_ORPHANS && han > 0) break;
    }
    if (!han) {
        printf("Error: no yaku hand\n");
        return -1;
    }
    if (dora > 0 && han < 13) {
        han += dora;
        printf("%-34s%-2u Han\n", "Dora", dora);
    }
    printf("%34c%-2u Han ", ' ', han);
    if (han >= 13) {
        unsigned char multiple = han / 13;
        if (multiple > 1) printf("(Multiple Yakuman)");
        else printf("(Yakuman)"); 
        score = multiple * 8000;
    } else if (han >= 11) {
        printf("(Sanbaiman)");
        score = 6000;
    } else if (han >= 8) {
        printf("(Baiman)");
        score = 4000;
    } else if (han >= 6) {
        printf("(Haneman)");
        score = 3000;
    } else if (han >= 5 || (han == 4 && fu >= 40) || (han == 3 && fu >= 70)) {
        printf("(Mangan)");
        score = 2000;
    } else {
        score = 1 << (2 + han);
        score *= fu;
    }
    putchar('\n');
    if (score_info->tsumo) {
        int rest = round_to_next(100, score);
        int dealer = round_to_next(100, score << 1);
        unsigned int honba, riichi;
        if (score_info->honba_counter) {
            honba = score_info->honba_counter * 100;
            printf("Honba Counter: %+i\n", honba);
            rest += honba;
            dealer += honba;
        }
        if (score_info->winner == EAST) {
            ++state->honba_counter;
            printf("Score: %i\n", dealer);
            for (int i = 0; i < state->num_players; ++i) {
                score_info->delta[i] = -dealer;
            }
            score_info->delta[0] = 3 * dealer;
        } else {
            printf("Score: (%i/%i)\n", rest, dealer);
            state->honba_counter = 0;
            for (int i = 0; i < state->num_players; ++i) {
                score_info->delta[i] = -rest;
            }
            score_info->delta[0] = -dealer;
            score_info->delta[score_info->winner - EAST] = 2 * rest + dealer;
        }
        if (score_info->riichi_sticks) {
            riichi = score_info->riichi_sticks * 1000;
            printf("Riichi-Sticks: %+i\n", riichi);
            state->riichi_sticks = 0;
            score_info->delta[score_info->winner - EAST] += riichi;
        }
    } else {
        score <<= 1;
        if (score_info->winner == EAST || score_info->from == EAST) score *= 3;
        else score <<= 1;
        if (score_info->winner == EAST) ++state->honba_counter;
        else state->honba_counter = 0;
        score = round_to_next(100, score);
        if (score_info->honba_counter) {
            unsigned int additional = score_info->honba_counter * 300;
            printf("Honba Counter: %+i\n", additional);
            score += additional;
        }
        printf("Score: %i\n", score);
        if (score_info->riichi_sticks) {
            unsigned int additional = score_info->riichi_sticks * 1000;
            printf("Riichi-Sticks: %+i\n", additional);
            score += additional;
            state->riichi_sticks = 0;
        }
        score_info->delta[score_info->winner - EAST] = score;
        score_info->delta[score_info->from - EAST] = -score;
    }
    return 0;
}

void handle_draw(struct score_info *score_info, struct state *state) {
    ++state->honba_counter;
    state->riichi_sticks += score_info->riichi_sticks;
    unsigned char number_tenpai = 0;
    for (int i = 0; i < state->num_players; ++i) {
        if (score_info->tenpai[i]) ++number_tenpai;
    }
    if (number_tenpai % state->num_players == 0) return;
    unsigned int receive = 3000 / number_tenpai;
    unsigned int pay = 3000 / (state->num_players - number_tenpai);
    for (int i = 0; i < state->num_players; ++i) {
        if (score_info->tenpai[i])
            score_info->delta[i] = receive;
        else
            score_info->delta[i] = -pay;
    }
}

void print_winner(const struct state *state) {
    int winner = 0;
    for (int i = 0; i < state->num_players; ++i) {
        if (state->points[i] > state->points[winner])
            winner = i;
    }
    printf("%s won!\n", state->players[winner]);
}

const char *directions[4] = {"East", "South", "West", "North"};

int main(void) {
    struct state state = {
        .num_players = 4,
        .round_wind = EAST,
        .round = 1,
        .dealer = 0,
        .points = {30000, 30000, 30000, 30000},
    };
    FILE *file = fopen("players.txt", "r");
    if (file == NULL) {
        printf("Error: can't load player names from file players.txt\n");
        exit(EXIT_FAILURE);
    }
    char players_buf[100];
    fgets(players_buf, sizeof(players_buf), file);
    fclose(file);
    char *player  = strtok(players_buf, ",");
    int i;
    for (i = 0; i < 4; ++i) {
        if (player == NULL) break;
        unsigned long len = strlen(player);
        if (player[len - 1] == '\n')
            player[len - 1] = 0;
        state.players[i] = player;
        player = strtok(NULL, ",");
    }
    if (i < 3) {
        printf("Error: can't have less than 3 players");
        exit(EXIT_FAILURE);
    }
    state.num_players = i;
    struct state_history history;
    init_state_history(&history, 10);
    push_state(&history, &state);
    history.push_depth = 0;
    struct score_info score_info;
    while (true) {
        printf("Round: %s %i\n", directions[state.round_wind - EAST], state.round);
        printf("Riichi-Sticks: %i\n", state.riichi_sticks);
        printf("Honba Counter: %i\n\n", state.honba_counter);
        print_scores(&score_info, &state, true);
        putchar('\n');
        int limit_hand = round_summary(&score_info, &state, &history);
        if (history.modified) {
            history.modified = false;
            printf("\n*--------------------------------------*\n\n");
            continue;
        }
        if (limit_hand < 0) break;
        else if (limit_hand > 0) {
            if (calculate_score(&score_info, &state) < 0) continue;
        } else if (score_info.draw) {
            handle_draw(&score_info, &state);
        } else {
            if ((score_info.dora = get_number("Enter number of Dora: ")) < 0) continue;
            if (get_hand(&score_info) < 0) continue;
            print_hand(&score_info);
            if (determine_wait(&score_info) < 0) continue;
            score_info.round_wind = state.round_wind;
            check_yaku(&score_info);
            calculate_fu(&score_info);
            score_info.honba_counter = state.honba_counter;
            score_info.riichi_sticks += state.riichi_sticks;
            if (calculate_score(&score_info, &state) < 0) continue;
        }
        print_scores(&score_info, &state, false);
        putchar('\n');
        if (score_info.winner != EAST && (!score_info.draw || !score_info.tenpai[0])) {
            if (state.round == state.num_players) {
                if (state.round_wind == SOUTH) {
                    printf("*--------------------------------------*\n\n");
                    print_scores(&score_info, &state, true);
                    putchar('\n');
                    print_winner(&state);
                    break;
                }
                ++state.round_wind;
                state.round = 1;
            } else {
                ++state.round;
            }
        }
        printf("*--------------------------------------*\n\n");
        push_state(&history, &state);
    }
    deinit_state_history(&history);
}
