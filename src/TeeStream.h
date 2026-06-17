#pragma once

#include <Stream.h>
#include <functional>

// Transparent Stream wrapper that intercepts complete <...> commands as they
// flow through read(), invoking a callback with the inner text (no angle brackets).
// Writes and all other Stream operations pass straight through to the inner stream.
class TeeStream : public Stream {
public:
  using CommandCallback = std::function<void(const char*)>;

  TeeStream(Stream& inner, CommandCallback onCommand)
      : _inner(inner), _onCommand(std::move(onCommand)) {}

  int    available() override                         { return _inner.available(); }
  int    peek()      override                         { return _inner.peek(); }
  void   flush()     override                         { _inner.flush(); }
  size_t write(uint8_t b) override                    { return _inner.write(b); }
  size_t write(const uint8_t* buf, size_t n) override { return _inner.write(buf, n); }

  int read() override {
    int c = _inner.read();
    if (c >= 0) _sniff((char)c);
    return c;
  }

private:
  Stream&         _inner;
  CommandCallback _onCommand;
  char            _buf[256];
  int             _bufLen = 0;
  bool            _inCmd  = false;

  void _sniff(char c) {
    if (c == '<') {
      _inCmd  = true;
      _bufLen = 0;
    } else if (c == '>') {
      if (_inCmd && _bufLen > 0) {
        _buf[_bufLen] = '\0';
        if (_onCommand) _onCommand(_buf);
      }
      _inCmd  = false;
      _bufLen = 0;
    } else if (_inCmd && _bufLen < (int)sizeof(_buf) - 1) {
      _buf[_bufLen++] = c;
    }
  }
};
