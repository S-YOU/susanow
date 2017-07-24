

#include <stdio.h>
#include <ssn_log.h>
#include <ssn_port.h>
#include <ssn_sys.h>
#include <ssn_common.h>

#include <stdio.h>
#include <string>

#include <slankdev/exception.h>
#include <slankdev/util.h>
#include <slankdev/extra/dpdk.h>

#include <ssn_sys.h>
#include <ssn_log.h>
#include <ssn_ring.h>
#include <ssn_port.h>
#include <ssn_port_stat.h>
#include <ssn_timer.h>
#include <ssn_native_thread.h>
#include <ssn_green_thread.h>

#include "vnf.h"
#include "vty_config.h"
#include "rest_config.h"

constexpr uint16_t vty_port = 9999;
constexpr uint16_t rest_port = 8888;
constexpr size_t   required_nb_cores = 4;
constexpr size_t   green_thread_lcoreid = 1;
constexpr size_t   timer_lcoreid = 2;

void print(void* arg)
{
  vnf* v = reinterpret_cast<vnf*>(arg);
  while (true) {
    v->debug_dump(stdout);
    printf("-------------\n");
    ssn_sleep(1000);
    ssn_yield();
  }
}

void init(ssn_timer_sched* timer_sched, ssn_vty* vty, ssn_rest* rest, int argc, char** argv);
void fini(ssn_timer_sched* timer_sched, ssn_vty* vty, ssn_rest* rest);

/*
 * VNF IMPLEMENTATION BEGIN
 */
class func_wk : public func {
  bool run;
 public:
  stageio_rx* prewk_[2];
  stageio_tx* poswk_[2];
  virtual void poll_exe() override
  {
    ssn_log(SSN_LOG_INFO, "func_wk: INCLUDE DELAY\r\n");
    size_t nb_ports = ssn_dev_count();
    rte_mbuf* mbufs[32];
    while (run) {
      for (size_t p=0; p<nb_ports; p++) {
        size_t deqlen = prewk_[p]->rx_burst(mbufs, 32);
        for (size_t i=0; i<deqlen; i++) {
          // for (size_t j=0; j<100; j++) ; // DELAY
          for (size_t j=0; j<30; j++) ; // DELAY
          int ret = poswk_[p^1]->tx_shot(mbufs[i]);
          if (ret < 0) rte_pktmbuf_free(mbufs[i]);
        }
      }
    }
  }
  virtual void stop() override { run = false; }
};

class func_rx : public func {
  bool run;
 public:
  stageio_rx* port_[2];
  stageio_tx* prewk_[2];
  virtual void poll_exe() override
  {
    size_t nb_ports = ssn_dev_count();
    while (run) {
      for (size_t p=0; p<nb_ports; p++) {
        rte_mbuf* mbufs[32];
        size_t recvlen = port_[p]->rx_burst(mbufs, 32);
        if (recvlen == 0) continue;
        size_t enqlen = prewk_[p]->tx_burst(mbufs, recvlen);
        if (recvlen > enqlen) {
          slankdev::rte_pktmbuf_free_bulk(&mbufs[enqlen], recvlen-enqlen);
        }
      }
    }
  }
  virtual void stop() override { run = false; }
};

class func_tx : public func {
  bool run;
 public:
  func_tx(stageio_rx* rx, stageio_tx* tx) :
  stageio_rx* poswk_[2];
  stageio_tx* port_[2];
  virtual void poll_exe() override
  {
    size_t nb_ports = ssn_dev_count();
    while (run) {
      for (size_t p=0; p<nb_ports; p++) {
        rte_mbuf* mbufs[32];
        size_t deqlen = poswk_[p]->rx_burst(mbufs, 32);
        if (deqlen == 0) continue;
        size_t sendlen = port_[p]->tx_burst(mbufs, deqlen);
        if (deqlen > sendlen) {
          slankdev::rte_pktmbuf_free_bulk(&mbufs[sendlen], deqlen-sendlen);
        }
      }
    }
  }
  virtual void stop() override { run = false; }
};


class stage_rx : public stage {
 public:
  stageio_rx_port port_[2];
  stageio_tx_ring prewk_[2];
  stage_rx(const char* n) : stage(n) {}
  virtual func* allocate() override
  {
    func_rx* rx = new func_rx;
    rx->port_[0]  = &port_[0];
    rx->port_[1]  = &port_[1];
    rx->prewk_[0] = &prewk_[0];
    rx->prewk_[1] = &prewk_[1];
    return rx;
  }
  virtual size_t throughput_pps() const override
  {
    size_t sum_rx_pps = 0;
    size_t nb_ports = ssn_dev_count();
    for (size_t i=0; i<nb_ports; i++) {
      sum_rx_pps += port_[i].rx_pps();
    }
    return sum_rx_pps;
  }
};

class stage_wk : public stage {
 public:
  stage_wk(const char* n) : stage(n) {}
  stageio_rx_ring prewk_[2];
  stageio_tx_ring poswk_[2];
  virtual func* allocate() override
  {
    func_wk* wk = new func_wk;
    wk->prewk_[0] = &prewk_[0];
    wk->prewk_[1] = &prewk_[1];
    wk->poswk_[0] = &poswk_[0];
    wk->poswk_[1] = &poswk_[1];
    return wk;
  }
  virtual size_t throughput_pps() const override
  {
    size_t sum_pps = 0;
    size_t nb_ports = ssn_dev_count();
    for (size_t i=0; i<nb_ports; i++) {
      sum_pps += prewk_[i].rx_pps();
    }
    return sum_pps;
  }
};

class stage_tx : public stage {
 public:
  stage_tx(const char* n) : stage(n) {}
  stageio_rx_ring poswk_[2];
  stageio_tx_port port_[2];

  virtual func* allocate() override
  {
    func_tx* tx = new func_tx;
    tx->poswk_[0] = &poswk_[0];
    tx->poswk_[1] = &poswk_[1];
    tx->port_[0]  = &port_[0];
    tx->port_[1]  = &port_[2];
    return tx;
  }
  virtual size_t throughput_pps() const override
  {
    size_t sum_pps = 0;
    size_t nb_ports = ssn_dev_count();
    for (size_t i=0; i<nb_ports; i++) {
      sum_pps += poswk_[i].rx_pps();
    }
    return sum_pps;
  }
};

class vnf_l2fwd : public vnf {
 public:
  ssn_ring* ring_prewk[2];
  ssn_ring* ring_poswk[2];
  vnf_l2fwd()
  {
    ring_prewk[0] = new ssn_ring("prewk0");
    ring_prewk[1] = new ssn_ring("prewk1");
    ring_poswk[0] = new ssn_ring("poswk0");
    ring_poswk[1] = new ssn_ring("poswk1");

    stage_rx* rx = new stage_rx("rx");
    rx->port_[0].pid = 0;
    rx->port_[0].qid = 0;
    rx->port_[1].pid = 1;
    rx->port_[1].qid = 0;
    rx->prewk_[0].ring = ring_prewk[0];
    rx->prewk_[1].ring = ring_prewk[1];

    stage_wk* wk = new stage_wk("wk");
    wk->prewk_[0].ring = ring_prewk[0];
    wk->prewk_[1].ring = ring_prewk[1];
    wk->poswk_[0].ring = ring_poswk[0];
    wk->poswk_[1].ring = ring_poswk[1];

    stage_tx* tx = new stage_tx("tx");
    tx->poswk_[0].ring = ring_poswk[0];
    tx->poswk_[1].ring = ring_poswk[1];
    tx->port_[0].pid = 0;
    tx->port_[0].qid = 0;
    tx->port_[1].pid = 1;
    tx->port_[1].qid = 0;

    stages.push_back(rx);
    stages.push_back(wk);
    stages.push_back(tx);
  }
};
/*
 * VNF IMPLEMENTATION END
 */



int main(int argc, char** argv)
{
  ssn_timer_sched* timer_sched;
  ssn_vty* vty;
  ssn_rest* rest;

  init(timer_sched, vty, rest, argc, argv);

  /* Init VNF */
  vnf_l2fwd* vnf1 = new vnf_l2fwd;
  vnf1->deploy();
  ssn_green_thread_launch(print, vnf1, green_thread_lcoreid);

  /* Loop */
  while (true) {
    char c = getchar();
    if (c == 'q') {
      printf("quit\n");
      break;
    }
    vnf1->tuneup();
  }

  fini(timer_sched, vty, rest);
}

void init(ssn_timer_sched* timer_sched, ssn_vty* vty, ssn_rest* rest, int argc, char** argv)
{
  ssn_log_set_level(SSN_LOG_EMERG);

  /*
   * Main Routine
   */
  ssn_init(argc, argv);
  if (ssn_lcore_count() <= required_nb_cores)
    throw slankdev::exception("cpu tarinai");

  /* Init Green Thread Scheduler */
  ssn_green_thread_sched_register(1);

  /* Init Timer Scheduler */
  timer_sched = new ssn_timer_sched(2);
  ssn_native_thread_launch(ssn_timer_sched_poll_thread, timer_sched, 2);
  uint64_t hz = ssn_timer_get_hz();
  timer_sched->add(new ssn_timer(ssn_port_stat_update, nullptr, hz));
  timer_sched->add(new ssn_timer(ssn_ring_getstats_timer_cb, nullptr, hz));

  /* Init VTY */
  vty = new ssn_vty(0x0, vty_port);
  vty->install_command(vtymt_slank()    , vtycb_slank    , nullptr);
  vty->install_command(vtymt_show_port(), vtycb_show_port, nullptr);
  vty->install_command(vtymt_show_ring(), vtycb_show_ring, nullptr );
  // vty->install_command(vtymt_show_vnf() , vtycb_show_vnf , &vnf0  );
  ssn_green_thread_launch(ssn_vty_poll_thread, vty, 1);

  /* Init REST API Server */
  rest = new ssn_rest(0x0, rest_port);
  rest->add_route("/"       , restcb_root  , nullptr);
  rest->add_route("/stats"  , restcb_stats , nullptr);
  rest->add_route("/author" , restcb_author, nullptr);
  ssn_green_thread_launch(ssn_rest_poll_thread, rest,1);

  /* Port Configuration */
  ssn_port_conf conf;
  size_t nb_ports = ssn_dev_count();
  for (size_t i=0; i<nb_ports; i++) {
    ssn_port_configure(i, &conf);
    ssn_port_dev_up(i);
    ssn_port_promisc_on(i);
  }
}

void fini(ssn_timer_sched* timer_sched, ssn_vty* vty, ssn_rest* rest)
{
  /*
   * Finilize
   */
  ssn_wait_all_lcore();
  ssn_green_thread_sched_unregister(1);
  delete timer_sched;
  ssn_wait_all_lcore();
  ssn_fin();
}
