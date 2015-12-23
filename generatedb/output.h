class l_class_OC_Fifo {
private:
public:
  void deq(void);
  bool deq__RDY(void);
  void enq(unsigned int enq_v);
  bool enq__RDY(void);
  unsigned int first(void);
  bool first__RDY(void);
};

class l_class_OC_Fifo1 {
private:
  unsigned int element;
  bool full;
public:
  void deq(void);
  bool deq__RDY(void);
  void enq(unsigned int enq_v);
  bool enq__RDY(void);
  unsigned int first(void);
  bool first__RDY(void);
};

class l_class_OC_Fifo_OC_1 {
private:
public:
  void deq(void);
  bool deq__RDY(void);
  void enq(bool enq_v);
  bool enq__RDY(void);
  bool first(void);
  bool first__RDY(void);
};

class l_class_OC_Fifo1_OC_0 {
private:
  bool element;
  bool full;
public:
  void deq(void);
  bool deq__RDY(void);
  void enq(bool enq_v);
  bool enq__RDY(void);
  bool first(void);
  bool first__RDY(void);
};

