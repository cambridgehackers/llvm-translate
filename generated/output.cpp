

/* Global Variable Definitions and Initialization */
class l_class_OC_EchoTest echoTest;
unsigned int stop_main_program;
class l_class_OC_Module *_ZN6Module5firstE;
//processing _ZN14EchoIndication4echoEi
void _ZN14EchoIndication4echoEi(unsigned int Vv) {
        stop_main_program = 1;
}
typedef struct {
    bool (*RDY)(void);
    void (*ENA)(void);
    } RuleVTab;//Rules:
const RuleVTab ruleList[] = {
    {_ZN4Echo7respond8respond23RDYEv, _ZN4Echo7respond8respond23ENAEv},
    {_ZN4Echo7respond8respond13RDYEv, _ZN4Echo7respond8respond13ENAEv},
    {_ZN8EchoTest5drive3RDYEv, _ZN8EchoTest5drive3ENAEv},
    {} };
