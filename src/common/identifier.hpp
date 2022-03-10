#pragma once

#define OM_INTEGER_IDENTIFIER_EQUALITY(class_name, id_member)                 \
  friend inline bool operator==(const class_name& a, const class_name& b) {   \
    return a.id_member == b.id_member;                                        \
  }                                                                           \
  friend inline bool operator!=(const class_name& a, const class_name& b) {   \
    return a.id_member != b.id_member;                                        \
  }