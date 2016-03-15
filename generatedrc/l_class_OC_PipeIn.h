#ifndef __PI__
#define __PI__
class l_class_OC_PipeIn {
    void *p;
    GUARDPTR enq__RDYp;
    void (*enqp)(void *p, l_struct_OC_EchoRequest_data v);
 public:
    METHOD(enq, (l_struct_OC_EchoRequest_data v), {return enq__RDYp(p); } ) { enqp(p, v); }
    l_class_OC_PipeIn(void *ap, GUARDPTR aenq__RDYp, decltype(enqp) aenqp) {
        p = ap;
        enq__RDYp = aenq__RDYp;
        enqp = aenqp;
    }
};
#endif
