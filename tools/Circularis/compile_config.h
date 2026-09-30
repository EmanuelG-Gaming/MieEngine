#ifndef CCL_COMPILE_CONFIG_H_
#define CCL_COMPILE_CONFIG_H_ 1

/*
   Circularis language.
*/

#define CCL_NON_NAMESPACE 1

#define CCL_NON_USING_NAMESPACE 1

#ifdef CCL_NON_NAMESPACE
#ifdef CCL_NON_USING_NAMESPACE
    #define ccl
#endif
#endif

#endif /* CCL_COMPILE_CONFIG_H_ */
