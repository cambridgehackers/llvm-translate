#ifndef __PI0__
#define __PI0__
class l_class_OC_PipeIn_OC_0 {
    void *p;
    GUARDPTR enq__RDYp;
    void (*enqp)(void *p, l_struct_OC_EchoIndication_data v);
 public:
    METHOD(enq, (l_struct_OC_EchoIndication_data v), {return enq__RDYp(p); } ) { enqp(p, v); }
    l_class_OC_PipeIn_OC_0(void *ap, GUARDPTR aenq__RDYp, decltype(enqp) aenqp) {
        p = ap;
        enq__RDYp = aenq__RDYp;
        enqp = aenqp;
    }
};
#endif
