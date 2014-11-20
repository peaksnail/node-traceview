#include "../node-oboe.h"

using namespace v8;

Persistent<FunctionTemplate> UdpReporter::constructor;

// Construct with an address and port to report to
UdpReporter::UdpReporter() {
  host = strdup("localhost");
  port = strdup("7831");
}

// Remember to cleanup the udp reporter struct when garbage collected
UdpReporter::~UdpReporter() {
  if (&reporter != NULL) {
    oboe_reporter_destroy(&reporter);
  }
}

int UdpReporter::connect() {
  return oboe_reporter_udp_init(&reporter, host, port);
}

int UdpReporter::send(oboe_metadata_t* meta, oboe_event_t* event) {
  return oboe_reporter_send(&reporter, meta, event);
}

NAN_SETTER(UdpReporter::setAddress) {
  NanScope();

  UdpReporter* self = ObjectWrap::Unwrap<UdpReporter>(args.This());

  if (&self->reporter != NULL) {
    oboe_reporter_destroy(&self->reporter);
  }

  if ( ! value->IsString()) {
    NanThrowError("Address must be a string");
    return;
  }

  std::string s = *NanUtf8String(value);
  char* host = strdup(s.substr(0, s.find(":")).c_str());
  char* port = strdup(s.substr(s.find(":") + 1).c_str());
  if (host == NULL || port == NULL) {
    NanThrowError("Invalid address string");
    return;
  }

  self->host = host;
  self->port = port;

  if (self->connect() != 0) {
    NanThrowError("UdpReporter() Failed to connect");
    return;
  }
}
NAN_GETTER(UdpReporter::getAddress) {
  NanScope();

  UdpReporter* self = ObjectWrap::Unwrap<UdpReporter>(args.This());
  std::string host = self->host;
  std::string port = self->port;
  std::string address = host + ":" + port;
  NanReturnValue(NanNew<String>(address));
}

NAN_SETTER(UdpReporter::setHost) {
  NanScope();

  UdpReporter* self = ObjectWrap::Unwrap<UdpReporter>(args.This());

  if (&self->reporter != NULL) {
    oboe_reporter_destroy(&self->reporter);
  }

  if ( ! value->IsString()) {
    NanThrowError("host must be a string");
    return;
  }
  self->host = *NanUtf8String(value);

  if (self->connect() != 0) {
    NanThrowError("UdpReporter() Failed to connect");
    return;
  }
}
NAN_GETTER(UdpReporter::getHost) {
  NanScope();

  UdpReporter* self = ObjectWrap::Unwrap<UdpReporter>(args.This());
  NanReturnValue(NanNew<String>(self->host));
}

NAN_SETTER(UdpReporter::setPort) {
  NanScope();

  UdpReporter* self = ObjectWrap::Unwrap<UdpReporter>(args.This());

  if (&self->reporter != NULL) {
    oboe_reporter_destroy(&self->reporter);
  }

  if ( ! value->IsString() && ! value->IsNumber()) {
    NanThrowError("port must be a string");
    return;
  }

  NanUtf8String val(value->ToString());
  self->port = *val;

  if (self->connect() != 0) {
    NanThrowError("UdpReporter() Failed to connect");
    return;
  }
}
NAN_GETTER(UdpReporter::getPort) {
  NanScope();

  UdpReporter* self = ObjectWrap::Unwrap<UdpReporter>(args.This());
  NanReturnValue(NanNew<String>(self->port));
}

// Transform a string back into a metadata instance
NAN_METHOD(UdpReporter::sendReport) {
  NanScope();

  if (args.Length() < 1) {
    return NanThrowError("Wrong number of arguments");
  }
  if (!args[0]->IsObject()) {
    return NanThrowError("Must supply an event instance");
  }

  UdpReporter* self = ObjectWrap::Unwrap<UdpReporter>(args.This());
  Event* event = ObjectWrap::Unwrap<Event>(args[0]->ToObject());

  oboe_metadata_t *md;
  if (args.Length() == 2 && args[1]->IsObject()) {
    Metadata* metadata = ObjectWrap::Unwrap<Metadata>(args[1]->ToObject());
    md = &metadata->metadata;
  } else {
    md = OboeContext::get();
  }

  int status = self->send(md, &event->event);
  NanReturnValue(NanNew<Boolean>(status >= 0));
}

// Creates a new Javascript instance
NAN_METHOD(UdpReporter::New) {
  NanScope();

  if (!args.IsConstructCall()) {
    return NanThrowError("UdpReporter() must be called as a constructor");
  }

  UdpReporter* reporter = new UdpReporter();
  if (reporter->connect() != 0) {
    return NanThrowError("UdpReporter() failed to connect");
  }

  reporter->Wrap(args.This());
  NanReturnValue(args.This());
}

// Wrap the C++ object so V8 can understand it
void UdpReporter::Init(Handle<Object> exports) {
	NanScope();

  // Prepare constructor template
  Handle<FunctionTemplate> ctor = NanNew<FunctionTemplate>(New);
  ctor->InstanceTemplate()->SetInternalFieldCount(1);
  ctor->SetClassName(NanNew<String>("UdpReporter"));
  NanAssignPersistent(constructor, ctor);

  // Assign host/port to change reporter target
  Local<ObjectTemplate> proto = ctor->PrototypeTemplate();
  proto->SetAccessor(NanNew<String>("address"), NULL, setAddress);
  proto->SetAccessor(NanNew<String>("host"), getHost, setHost);
  proto->SetAccessor(NanNew<String>("port"), getPort, setPort);

  // Prototype
  NODE_SET_PROTOTYPE_METHOD(ctor, "sendReport", UdpReporter::sendReport);

  exports->Set(NanNew<String>("UdpReporter"), ctor->GetFunction());
}
