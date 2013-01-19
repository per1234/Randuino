/**
 * Randuino - A TRNG library for Arduino
 * Copyright (C) 2011 Carlo Alberto Ferraris <cafxx@strayorange.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef trng_h
#define trng_h

#include "WProgram.h"
#include "sha256.h"

#define inputPin 0

namespace {

  class randuino : private Sha256Class {
    static unsigned long ctr;
    const unsigned int min_iterations;
    unsigned int iterations;
    enum states { NOT_INITED, NOT_READY, READY } state;
    
    public:
      randuino(unsigned int _min_iterations = HASH_LENGTH * 8) : min_iterations(_min_iterations) {
        state = NOT_INITED;
      }
      
      void init() {
        if (bg_generator_is_enabled())
          return bg->init();
          
        Sha256Class::init();
        write(ctr);
        write(ctr>>8);
        write(ctr>>16);
        write(ctr>>24);
        ctr++;
        iterations = 0;
        state = NOT_READY;
      }
      
      uint8_t* result(unsigned int _min_iterations = 0) {
        if (_min_iterations == 0)
          _min_iterations = min_iterations;
        if (bg_generator_is_enabled())
          return bg->result(_min_iterations);
          
        if (!is_ready(_min_iterations)) 
          return NULL;
        state = NOT_INITED;
        return Sha256Class::result();
      }
      
      boolean is_ready(unsigned int _min_iterations = 0) {
        if (_min_iterations == 0)
          _min_iterations = min_iterations;
        if (bg_generator_is_enabled())
          return bg->is_ready(_min_iterations);
          
        if (state == NOT_INITED)
          return false;
        boolean ready = iterations >= _min_iterations;
        state = ready ? READY : NOT_READY;
        return ready;
      }
      
      void step() {
        if (bg_generator_is_enabled())
          return;

        if (state == NOT_INITED)
          init();
        unsigned int in = analogRead(inputPin);
        write(in ^ (in >> 8));
        iterations++;
      }
      
      static int rand(unsigned char *out, unsigned int size, unsigned int iter) {
        randuino r(iter);
        uint8_t *res;
        unsigned int i = 0;
        
        while (i < size) {
          while (!(res = r.result()))
            r.step();
          memcpy(out+i, res, min(HASH_LENGTH, size-i));
          i += HASH_LENGTH;
        }
        return 0;
      }
      
    private:
      static randuino* bg;
      
    public:      
      static boolean enable_bg_generator() {
        if (bg_generator_is_enabled())
          disable_bg_generator();
        bg = new randuino();
        return bg_generator_is_enabled();
      }
      
      static boolean disable_bg_generator() {
        if (bg != NULL)
          delete bg;
        bg = NULL;
        return bg_generator_is_enabled();
      }
      
      static boolean bg_generator_is_enabled() {
        return false;
        return bg != NULL;
      }
  };
  
  unsigned long randuino::ctr = 0;
  randuino* randuino::bg = NULL;

}

#endif /* trng_h */