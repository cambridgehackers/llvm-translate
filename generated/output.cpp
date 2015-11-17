

/* Global Variable Definitions and Initialization */
class l_class_OC_EchoTest echoTest;
unsigned int stop_main_program;
class l_class_OC_Module *_ZN6Module5firstE;
//processing _ZN14EchoIndication4echoEi
void _ZN14EchoIndication4echoEi(unsigned int Vv) {
        printf((("Heard an echo: %d\n")), Vv);
        stop_main_program = 1;
}
//processing printf
//processing _ZN14EchoIndication4echoEi
typedef struct {
    bool (*RDY)(void);
    void (*ENA)(void);
    } RuleVTab;//Rules:
const RuleVTab ruleList[] = {
    {} };
bool l_class_OC_EchoTest_KD__KD_drive::RDY(void) {
    bool tmp__1 =     ((*(((module)->echo)->fifo)).enq__RDY)();
        return tmp__1;
}
void l_class_OC_EchoTest_KD__KD_drive::ENA(void) {
        ((*(((module)->echo)->fifo)).enq)(22);
}
void l_class_OC_EchoTest_KD__KD_drive::run()
{
    if (RDY()) ENA();
}
void l_class_OC_EchoTest::run()
{
    echo->run();
    driveRule.run();
}
bool l_class_OC_Echo_KD__KD_respond_KD__KD_respond1::RDY(void) {
    bool tmp__1 =     ((*((module)->fifo)).deq__RDY)();
    bool tmp__2 =     ((*((module)->fifo)).first__RDY)();
        return (tmp__1 & tmp__2);
}
void l_class_OC_Echo_KD__KD_respond_KD__KD_respond1::ENA(void) {
        ((*((module)->fifo)).deq)();
    unsigned int call =     ((*((module)->fifo)).first)();
        _ZN14EchoIndication4echoEi(call);
}
void l_class_OC_Echo_KD__KD_respond_KD__KD_respond1::run()
{
    if (RDY()) ENA();
}
void l_class_OC_Echo_KD__KD_respond_KD__KD_respond2::ENA(void) {
}
bool l_class_OC_Echo_KD__KD_respond_KD__KD_respond2::RDY(void) {
        return 1;
}
void l_class_OC_Echo_KD__KD_respond_KD__KD_respond2::run()
{
    if (RDY()) ENA();
}
void l_class_OC_Echo_KD__KD_respond::run()
{
    respond1Rule.run();
    respond2Rule.run();
}
void l_class_OC_Echo::run()
{
    respondRule.run();
}
bool l_class_OC_Fifo1::enq__RDY(void) {
        return ((full) ^ 1);
}
void l_class_OC_Fifo1::enq(unsigned int v) {
        (element) = v;
        (full) = 1;
}
bool l_class_OC_Fifo1::deq__RDY(void) {
        return (full);
}
void l_class_OC_Fifo1::deq(void) {
        (full) = 0;
}
bool l_class_OC_Fifo1::first__RDY(void) {
        return (full);
}
unsigned int l_class_OC_Fifo1::first(void) {
        return (element);
}
