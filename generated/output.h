class l_class_OC_Module {
public:
};

class l_class_OC_Fifo {
public:
};

class l_class_OC_EchoIndication {
public:
  unsigned long long unused_data_to_flag_indication_echo;
  void echo(unsigned int Vv);
};

class l_class_OC_Fifo1 {
public:
  unsigned int element;
  bool full;
  bool enq__RDY(void);
  void enq(unsigned int Vv);
  bool deq__RDY(void);
  void deq(void);
  bool first__RDY(void);
  unsigned int first(void);
  bool notEmpty(void);
  bool notFull(void);
};

class l_class_OC_Echo {
public:
  class l_class_OC_Fifo1 fifo;
  class l_class_OC_EchoIndication *ind;
  unsigned int pipetemp;
  bool rule_respond__RDY(void);
  bool echoReq__RDY(void);
  void echoReq(unsigned int Vv);
  void rule_respond(void);
  void run();
};

class l_class_OC_EchoTest {
public:
  class l_class_OC_Echo *echo;
  unsigned int x;
};

