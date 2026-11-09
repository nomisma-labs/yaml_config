// SPDX-FileCopyrightText: 2026 Alexey Tkachenko
// SPDX-License-Identifier: LicenseRef-Nomisma-Labs-Proprietary

#pragma once

// Apply a statement macro to up to 64 members.
#define NOM_YCFG_FOREACH_1(M, x) M(x)
#define NOM_YCFG_FOREACH_2(M, x, ...) M(x) NOM_YCFG_FOREACH_1(M, __VA_ARGS__)
#define NOM_YCFG_FOREACH_3(M, x, ...) M(x) NOM_YCFG_FOREACH_2(M, __VA_ARGS__)
#define NOM_YCFG_FOREACH_4(M, x, ...) M(x) NOM_YCFG_FOREACH_3(M, __VA_ARGS__)
#define NOM_YCFG_FOREACH_5(M, x, ...) M(x) NOM_YCFG_FOREACH_4(M, __VA_ARGS__)
#define NOM_YCFG_FOREACH_6(M, x, ...) M(x) NOM_YCFG_FOREACH_5(M, __VA_ARGS__)
#define NOM_YCFG_FOREACH_7(M, x, ...) M(x) NOM_YCFG_FOREACH_6(M, __VA_ARGS__)
#define NOM_YCFG_FOREACH_8(M, x, ...) M(x) NOM_YCFG_FOREACH_7(M, __VA_ARGS__)
#define NOM_YCFG_FOREACH_9(M, x, ...) M(x) NOM_YCFG_FOREACH_8(M, __VA_ARGS__)
#define NOM_YCFG_FOREACH_10(M, x, ...) M(x) NOM_YCFG_FOREACH_9(M, __VA_ARGS__)
#define NOM_YCFG_FOREACH_11(M, x, ...) M(x) NOM_YCFG_FOREACH_10(M, __VA_ARGS__)
#define NOM_YCFG_FOREACH_12(M, x, ...) M(x) NOM_YCFG_FOREACH_11(M, __VA_ARGS__)
#define NOM_YCFG_FOREACH_13(M, x, ...) M(x) NOM_YCFG_FOREACH_12(M, __VA_ARGS__)
#define NOM_YCFG_FOREACH_14(M, x, ...) M(x) NOM_YCFG_FOREACH_13(M, __VA_ARGS__)
#define NOM_YCFG_FOREACH_15(M, x, ...) M(x) NOM_YCFG_FOREACH_14(M, __VA_ARGS__)
#define NOM_YCFG_FOREACH_16(M, x, ...) M(x) NOM_YCFG_FOREACH_15(M, __VA_ARGS__)
#define NOM_YCFG_FOREACH_17(M, x, ...) M(x) NOM_YCFG_FOREACH_16(M, __VA_ARGS__)
#define NOM_YCFG_FOREACH_18(M, x, ...) M(x) NOM_YCFG_FOREACH_17(M, __VA_ARGS__)
#define NOM_YCFG_FOREACH_19(M, x, ...) M(x) NOM_YCFG_FOREACH_18(M, __VA_ARGS__)
#define NOM_YCFG_FOREACH_20(M, x, ...) M(x) NOM_YCFG_FOREACH_19(M, __VA_ARGS__)
#define NOM_YCFG_FOREACH_21(M, x, ...) M(x) NOM_YCFG_FOREACH_20(M, __VA_ARGS__)
#define NOM_YCFG_FOREACH_22(M, x, ...) M(x) NOM_YCFG_FOREACH_21(M, __VA_ARGS__)
#define NOM_YCFG_FOREACH_23(M, x, ...) M(x) NOM_YCFG_FOREACH_22(M, __VA_ARGS__)
#define NOM_YCFG_FOREACH_24(M, x, ...) M(x) NOM_YCFG_FOREACH_23(M, __VA_ARGS__)
#define NOM_YCFG_FOREACH_25(M, x, ...) M(x) NOM_YCFG_FOREACH_24(M, __VA_ARGS__)
#define NOM_YCFG_FOREACH_26(M, x, ...) M(x) NOM_YCFG_FOREACH_25(M, __VA_ARGS__)
#define NOM_YCFG_FOREACH_27(M, x, ...) M(x) NOM_YCFG_FOREACH_26(M, __VA_ARGS__)
#define NOM_YCFG_FOREACH_28(M, x, ...) M(x) NOM_YCFG_FOREACH_27(M, __VA_ARGS__)
#define NOM_YCFG_FOREACH_29(M, x, ...) M(x) NOM_YCFG_FOREACH_28(M, __VA_ARGS__)
#define NOM_YCFG_FOREACH_30(M, x, ...) M(x) NOM_YCFG_FOREACH_29(M, __VA_ARGS__)
#define NOM_YCFG_FOREACH_31(M, x, ...) M(x) NOM_YCFG_FOREACH_30(M, __VA_ARGS__)
#define NOM_YCFG_FOREACH_32(M, x, ...) M(x) NOM_YCFG_FOREACH_31(M, __VA_ARGS__)
#define NOM_YCFG_FOREACH_33(M, x, ...) M(x) NOM_YCFG_FOREACH_32(M, __VA_ARGS__)
#define NOM_YCFG_FOREACH_34(M, x, ...) M(x) NOM_YCFG_FOREACH_33(M, __VA_ARGS__)
#define NOM_YCFG_FOREACH_35(M, x, ...) M(x) NOM_YCFG_FOREACH_34(M, __VA_ARGS__)
#define NOM_YCFG_FOREACH_36(M, x, ...) M(x) NOM_YCFG_FOREACH_35(M, __VA_ARGS__)
#define NOM_YCFG_FOREACH_37(M, x, ...) M(x) NOM_YCFG_FOREACH_36(M, __VA_ARGS__)
#define NOM_YCFG_FOREACH_38(M, x, ...) M(x) NOM_YCFG_FOREACH_37(M, __VA_ARGS__)
#define NOM_YCFG_FOREACH_39(M, x, ...) M(x) NOM_YCFG_FOREACH_38(M, __VA_ARGS__)
#define NOM_YCFG_FOREACH_40(M, x, ...) M(x) NOM_YCFG_FOREACH_39(M, __VA_ARGS__)
#define NOM_YCFG_FOREACH_41(M, x, ...) M(x) NOM_YCFG_FOREACH_40(M, __VA_ARGS__)
#define NOM_YCFG_FOREACH_42(M, x, ...) M(x) NOM_YCFG_FOREACH_41(M, __VA_ARGS__)
#define NOM_YCFG_FOREACH_43(M, x, ...) M(x) NOM_YCFG_FOREACH_42(M, __VA_ARGS__)
#define NOM_YCFG_FOREACH_44(M, x, ...) M(x) NOM_YCFG_FOREACH_43(M, __VA_ARGS__)
#define NOM_YCFG_FOREACH_45(M, x, ...) M(x) NOM_YCFG_FOREACH_44(M, __VA_ARGS__)
#define NOM_YCFG_FOREACH_46(M, x, ...) M(x) NOM_YCFG_FOREACH_45(M, __VA_ARGS__)
#define NOM_YCFG_FOREACH_47(M, x, ...) M(x) NOM_YCFG_FOREACH_46(M, __VA_ARGS__)
#define NOM_YCFG_FOREACH_48(M, x, ...) M(x) NOM_YCFG_FOREACH_47(M, __VA_ARGS__)
#define NOM_YCFG_FOREACH_49(M, x, ...) M(x) NOM_YCFG_FOREACH_48(M, __VA_ARGS__)
#define NOM_YCFG_FOREACH_50(M, x, ...) M(x) NOM_YCFG_FOREACH_49(M, __VA_ARGS__)
#define NOM_YCFG_FOREACH_51(M, x, ...) M(x) NOM_YCFG_FOREACH_50(M, __VA_ARGS__)
#define NOM_YCFG_FOREACH_52(M, x, ...) M(x) NOM_YCFG_FOREACH_51(M, __VA_ARGS__)
#define NOM_YCFG_FOREACH_53(M, x, ...) M(x) NOM_YCFG_FOREACH_52(M, __VA_ARGS__)
#define NOM_YCFG_FOREACH_54(M, x, ...) M(x) NOM_YCFG_FOREACH_53(M, __VA_ARGS__)
#define NOM_YCFG_FOREACH_55(M, x, ...) M(x) NOM_YCFG_FOREACH_54(M, __VA_ARGS__)
#define NOM_YCFG_FOREACH_56(M, x, ...) M(x) NOM_YCFG_FOREACH_55(M, __VA_ARGS__)
#define NOM_YCFG_FOREACH_57(M, x, ...) M(x) NOM_YCFG_FOREACH_56(M, __VA_ARGS__)
#define NOM_YCFG_FOREACH_58(M, x, ...) M(x) NOM_YCFG_FOREACH_57(M, __VA_ARGS__)
#define NOM_YCFG_FOREACH_59(M, x, ...) M(x) NOM_YCFG_FOREACH_58(M, __VA_ARGS__)
#define NOM_YCFG_FOREACH_60(M, x, ...) M(x) NOM_YCFG_FOREACH_59(M, __VA_ARGS__)
#define NOM_YCFG_FOREACH_61(M, x, ...) M(x) NOM_YCFG_FOREACH_60(M, __VA_ARGS__)
#define NOM_YCFG_FOREACH_62(M, x, ...) M(x) NOM_YCFG_FOREACH_61(M, __VA_ARGS__)
#define NOM_YCFG_FOREACH_63(M, x, ...) M(x) NOM_YCFG_FOREACH_62(M, __VA_ARGS__)
#define NOM_YCFG_FOREACH_64(M, x, ...) M(x) NOM_YCFG_FOREACH_63(M, __VA_ARGS__)
#define NOM_YCFG_SELECT(_1,                                                                                            \
                        _2,                                                                                            \
                        _3,                                                                                            \
                        _4,                                                                                            \
                        _5,                                                                                            \
                        _6,                                                                                            \
                        _7,                                                                                            \
                        _8,                                                                                            \
                        _9,                                                                                            \
                        _10,                                                                                           \
                        _11,                                                                                           \
                        _12,                                                                                           \
                        _13,                                                                                           \
                        _14,                                                                                           \
                        _15,                                                                                           \
                        _16,                                                                                           \
                        _17,                                                                                           \
                        _18,                                                                                           \
                        _19,                                                                                           \
                        _20,                                                                                           \
                        _21,                                                                                           \
                        _22,                                                                                           \
                        _23,                                                                                           \
                        _24,                                                                                           \
                        _25,                                                                                           \
                        _26,                                                                                           \
                        _27,                                                                                           \
                        _28,                                                                                           \
                        _29,                                                                                           \
                        _30,                                                                                           \
                        _31,                                                                                           \
                        _32,                                                                                           \
                        _33,                                                                                           \
                        _34,                                                                                           \
                        _35,                                                                                           \
                        _36,                                                                                           \
                        _37,                                                                                           \
                        _38,                                                                                           \
                        _39,                                                                                           \
                        _40,                                                                                           \
                        _41,                                                                                           \
                        _42,                                                                                           \
                        _43,                                                                                           \
                        _44,                                                                                           \
                        _45,                                                                                           \
                        _46,                                                                                           \
                        _47,                                                                                           \
                        _48,                                                                                           \
                        _49,                                                                                           \
                        _50,                                                                                           \
                        _51,                                                                                           \
                        _52,                                                                                           \
                        _53,                                                                                           \
                        _54,                                                                                           \
                        _55,                                                                                           \
                        _56,                                                                                           \
                        _57,                                                                                           \
                        _58,                                                                                           \
                        _59,                                                                                           \
                        _60,                                                                                           \
                        _61,                                                                                           \
                        _62,                                                                                           \
                        _63,                                                                                           \
                        _64,                                                                                           \
                        NAME,                                                                                          \
                        ...)                                                                                           \
    NAME
#define NOM_YCFG_FOR_EACH(M, ...)                                                                                      \
    NOM_YCFG_SELECT(__VA_ARGS__,                                                                                       \
                    NOM_YCFG_FOREACH_64,                                                                               \
                    NOM_YCFG_FOREACH_63,                                                                               \
                    NOM_YCFG_FOREACH_62,                                                                               \
                    NOM_YCFG_FOREACH_61,                                                                               \
                    NOM_YCFG_FOREACH_60,                                                                               \
                    NOM_YCFG_FOREACH_59,                                                                               \
                    NOM_YCFG_FOREACH_58,                                                                               \
                    NOM_YCFG_FOREACH_57,                                                                               \
                    NOM_YCFG_FOREACH_56,                                                                               \
                    NOM_YCFG_FOREACH_55,                                                                               \
                    NOM_YCFG_FOREACH_54,                                                                               \
                    NOM_YCFG_FOREACH_53,                                                                               \
                    NOM_YCFG_FOREACH_52,                                                                               \
                    NOM_YCFG_FOREACH_51,                                                                               \
                    NOM_YCFG_FOREACH_50,                                                                               \
                    NOM_YCFG_FOREACH_49,                                                                               \
                    NOM_YCFG_FOREACH_48,                                                                               \
                    NOM_YCFG_FOREACH_47,                                                                               \
                    NOM_YCFG_FOREACH_46,                                                                               \
                    NOM_YCFG_FOREACH_45,                                                                               \
                    NOM_YCFG_FOREACH_44,                                                                               \
                    NOM_YCFG_FOREACH_43,                                                                               \
                    NOM_YCFG_FOREACH_42,                                                                               \
                    NOM_YCFG_FOREACH_41,                                                                               \
                    NOM_YCFG_FOREACH_40,                                                                               \
                    NOM_YCFG_FOREACH_39,                                                                               \
                    NOM_YCFG_FOREACH_38,                                                                               \
                    NOM_YCFG_FOREACH_37,                                                                               \
                    NOM_YCFG_FOREACH_36,                                                                               \
                    NOM_YCFG_FOREACH_35,                                                                               \
                    NOM_YCFG_FOREACH_34,                                                                               \
                    NOM_YCFG_FOREACH_33,                                                                               \
                    NOM_YCFG_FOREACH_32,                                                                               \
                    NOM_YCFG_FOREACH_31,                                                                               \
                    NOM_YCFG_FOREACH_30,                                                                               \
                    NOM_YCFG_FOREACH_29,                                                                               \
                    NOM_YCFG_FOREACH_28,                                                                               \
                    NOM_YCFG_FOREACH_27,                                                                               \
                    NOM_YCFG_FOREACH_26,                                                                               \
                    NOM_YCFG_FOREACH_25,                                                                               \
                    NOM_YCFG_FOREACH_24,                                                                               \
                    NOM_YCFG_FOREACH_23,                                                                               \
                    NOM_YCFG_FOREACH_22,                                                                               \
                    NOM_YCFG_FOREACH_21,                                                                               \
                    NOM_YCFG_FOREACH_20,                                                                               \
                    NOM_YCFG_FOREACH_19,                                                                               \
                    NOM_YCFG_FOREACH_18,                                                                               \
                    NOM_YCFG_FOREACH_17,                                                                               \
                    NOM_YCFG_FOREACH_16,                                                                               \
                    NOM_YCFG_FOREACH_15,                                                                               \
                    NOM_YCFG_FOREACH_14,                                                                               \
                    NOM_YCFG_FOREACH_13,                                                                               \
                    NOM_YCFG_FOREACH_12,                                                                               \
                    NOM_YCFG_FOREACH_11,                                                                               \
                    NOM_YCFG_FOREACH_10,                                                                               \
                    NOM_YCFG_FOREACH_9,                                                                                \
                    NOM_YCFG_FOREACH_8,                                                                                \
                    NOM_YCFG_FOREACH_7,                                                                                \
                    NOM_YCFG_FOREACH_6,                                                                                \
                    NOM_YCFG_FOREACH_5,                                                                                \
                    NOM_YCFG_FOREACH_4,                                                                                \
                    NOM_YCFG_FOREACH_3,                                                                                \
                    NOM_YCFG_FOREACH_2,                                                                                \
                    NOM_YCFG_FOREACH_1)(M, __VA_ARGS__)
