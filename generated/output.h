class l_class_OC_Module {
public:
  unsigned long long unused_data_to_force_inheritance;
};

class l_class_OC_Fifo {
public:
  unsigned long long unused_data_to_force_inheritance;
};

class l_class_OC_EchoIndication {
public:
  unsigned long long unused_data_to_flag_indication_echo;
  void echo(unsigned int Vv);
};

class l_class_OC_Fifo1 {
public:
  unsigned long long unused_data_to_force_inheritance;
  unsigned int element;
  bool full;
  void enq(unsigned int Vv);
  bool deq__RDY(void);
  void deq(void);
  bool first__RDY(void);
  unsigned int first(void);
  bool notEmpty(void);
};

class l_class_OC_Echo {
public:
  unsigned long long unused_data_to_force_inheritance;
  class l_class_OC_Fifo1 fifo;
  class l_class_OC_EchoIndication *ind;
  unsigned int pipetemp;
  unsigned long long unused_data_to_flag_request_echoReq;
  bool echoReq__RDY(void);
  void echoReq(unsigned int Vv);
  void rule_respondexport(void);
  bool rule_respondexport__RDY(void);
  void rule_respond(void);
  bool rule_respond__RDY(void);
  void run();
};

class l_class_OC_EchoTest {
public:
  class l_class_OC_Echo *echo;
  unsigned int x;
};

