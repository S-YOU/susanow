

/*-
 * MIT License
 *
 * Copyright (c) 2017 Hiroki SHIROKURA
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
/**
 * @file ssnlib_port_impl.h
 * @brief include classes that provide port-infomation
 * @author slankdev
 */


#pragma once

#include <stdint.h>
#include <stddef.h>

#include <string>

#include <rte_ethdev.h>




class port_conf {
 public:
  const size_t id;
  rte_eth_conf raw;
  port_conf(size_t i);
};


class port_stats {
  static const size_t IFG = 12;  /* Inter Frame Gap [Byte] */
  static const size_t PAM =  8;  /* Preamble        [Byte] */
  static const size_t FCS =  4;  /* Frame Check Seq [Byte] */
  static const size_t IFO = IFG+PAM+FCS; /* Pack Overhead  */
 public:
  const size_t id;
  size_t rx_pps;
  size_t tx_pps;
  size_t rx_bps;
  size_t tx_bps;
  uint64_t last_update;
  struct rte_eth_stats init;
  struct rte_eth_stats cure_prev;
  struct rte_eth_stats cure;

  port_stats(size_t i)
    : id(i), rx_pps(0), tx_pps(0), rx_bps(0), tx_bps(0),
    last_update(0) { reset(); }
  void reset();
  void update();
};


class link_stats {
 public:
  const size_t id;
  struct rte_eth_link raw;
  link_stats(size_t i) : id(i) {}
  void update() { rte_eth_link_get_nowait(id, &raw); }
};


class dev_info {
 public:
  const size_t id;
  struct rte_eth_dev_info raw;
  dev_info(size_t i) : id(i) {}
  void get() { rte_eth_dev_info_get(id, &raw); }
};


class Ether_addr : public ::ether_addr {
 public:
  const size_t id;
  Ether_addr(size_t i) : id(i) {}
  void print(FILE* fd) const { fprintf(fd, "%s", toString().c_str()); }
  void update() { rte_eth_macaddr_get(id, this); }
  void set(::ether_addr* addr);
  void add(::ether_addr* addr);
  void del(::ether_addr* addr);
  std::string toString() const;
};




