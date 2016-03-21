#ifndef __l_struct_OC_ValuePair_H__
#define __l_struct_OC_ValuePair_H__
typedef struct {
public:
  unsigned int a, a_shadow; bool a_valid;
  unsigned int b, b_shadow; bool b_valid;
  unsigned int c, c_shadow[20]; bool c_valid;
}l_struct_OC_ValuePair;
#endif  // __l_struct_OC_ValuePair_H__
