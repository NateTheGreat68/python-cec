/* cec.cpp
 * This file provides the python cec module, which is a python wrapper
 *  around libcec
 *
 * Author: Austin Hendrix <namniart@gmail.com>
 */

#include <Python.h>
#include <stdlib.h>
#include <libcec/cec.h>
#include <list>

using namespace CEC;

// Basic design/usage:
//
// On module load:
// ( import cec )
//    call CECInitialize() and get THE ICECAdapter instance
//    call InitVideoStandalone
// 
// ( adapters = cec.list_adapters() )
//    call ICECAdapter::DetectAdapters
//
// ( cec.open( adapter = cec.default_adapter ) )
//    call DetectAdapters and choose the first adapter for default
//    call ICECAdapter::Open(...).
//
// ( cec.add_callback(event, handler) )
//    ???
//
// ( cec.remove_callback(event, handler) )
//    ???
//
// ( cec.transmit( ??? ) )
//    call ICECAdapter::Transmit()
//
// Events:
//    - log message
//       message.message[1024]
//       message.level (bitfield)
//       message.time (int64)
//    - key press
//       key.keycode (int/enum)
//       key.duration (int)
//    - command
//       command.initiator (logical address)
//       command.destination (logical address)
//       command.ack
//       command.eom
//       command.opcode (enum)
//       command.parameters 
//       command.opcode_set (flag for when opcode is set; should probably use
//          this to set opcode to None)
//       command.transmit_timeout
//    - cec configuration changed (adapter/library config change)
//    - alert
//    - menu state changed
//       this is potentially thorny; see libcec source and CEC spec
//       ???
//    - source activated
//

class CallbackList {
   private:
      std::list<PyObject*> callbacks;

   public:
      ~CallbackList() {
         // decrement the reference counts of everything in the list
         std::list<PyObject*>::const_iterator itr;
         for( itr = callbacks.begin(); itr != callbacks.end(); itr++ ) {
            Py_DECREF(*itr);
         }
      }

      void add(PyObject * cb) {
         assert(cb);
         Py_INCREF(cb);
         callbacks.push_back(cb);
      }

      PyObject * call(PyObject * args) {
         Py_INCREF(Py_None);
         PyObject * result = Py_None;

         std::list<PyObject*>::const_iterator itr;
         for( itr = callbacks.begin(); itr != callbacks.end(); itr++ ) {
            // see also: PyObject_CallFunction(...) which can take C args
            PyObject * temp = PyObject_CallObject(*itr, args);
            if( temp ) {
               Py_DECREF(temp);
            } else {
               Py_DECREF(Py_None);
               return NULL;
            }
         }
         return result;
      }
};

CallbackList * open_callbacks; // TODO: remove/update

ICECAdapter * CEC_adapter;

std::list<cec_adapter_descriptor> get_adapters() {
   // get adapters
   int cec_count = 10;
   cec_adapter_descriptor * dev_list = (cec_adapter_descriptor*)malloc(
         cec_count * sizeof(cec_adapter_descriptor));
   int count = CEC_adapter->DetectAdapters(dev_list, cec_count);
   if( count > cec_count ) {
      cec_count = count;
      dev_list = (cec_adapter_descriptor*)realloc(dev_list, 
         cec_count * sizeof(cec_adapter_descriptor));
      count = CEC_adapter->DetectAdapters(dev_list, cec_count);
      //if( 
   }

   //for( int i=0; i<
}

static PyObject * list_adapters(PyObject * self, PyObject * args) {
   PyObject * result = NULL;

   if( PyArg_ParseTuple(args, ":list_adapters") ) {
      // set up our result list
      result = PyList_New(0);

      // populate our result list
      // PyList_append(result, PyObject *)
   }

   return result;
}

static PyObject * open(PyObject * self, PyObject * args) {
   return open_callbacks->call(args);
}

static PyObject * add_callback(PyObject * self, PyObject * args) {
   PyObject * result = NULL;
   PyObject * temp;

   if( PyArg_ParseTuple(args, "O:add_callback", &temp) ) {
      if( !PyCallable_Check(temp)) {
         PyErr_SetString(PyExc_TypeError, "parameter must be callable");
         return NULL;
      }
      open_callbacks->add(temp);
      Py_INCREF(Py_None);
      result = Py_None;
   }
   return result;
}

static PyMethodDef CecMethods[] = {
   {"list_adapters", list_adapters, METH_VARARGS, "List available adapters"},
   {"open", open, METH_VARARGS, "Open an adapter"},
   {"add_callback", add_callback, METH_VARARGS, "Add a callback"},
   {NULL, NULL, 0, NULL}
};

libcec_configuration * CEC_config;
ICECCallbacks * CEC_callbacks; 

PyMODINIT_FUNC initcec(void) {
   // set up callback data structures
   open_callbacks = new CallbackList();

   // set up libcec
   //  libcec config
   CEC_config = new libcec_configuration();
   CEC_config->Clear();

   snprintf(CEC_config->strDeviceName, 13, "python-cec");
   CEC_config->clientVersion = CEC_CLIENT_VERSION_CURRENT;
   CEC_config->bActivateSource = 0;

   //  libcec callbacks
   CEC_callbacks = new ICECCallbacks();
   CEC_callbacks->Clear();

   CEC_config->callbacks = CEC_callbacks;

   CEC_adapter = (ICECAdapter*)CECInitialise(CEC_config);

   if( !CEC_adapter ) {
      // TODO: die
      return;
   }

   CEC_adapter->InitVideoStandalone();
   // TODO: initialize libcec here
   Py_InitModule("cec", CecMethods);
}
