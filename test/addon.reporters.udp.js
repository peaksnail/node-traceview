var helper = require('./helper')
var tv = require('..')
var addon = tv.addon

describe('addon.reporters.udp', function () {
  var emitter

  //
  // Intercept tracelyzer messages for analysis
  //
  before(function (done) {
    emitter = helper.tracelyzer(done)
    tv.sampleRate = tv.addon.MAX_SAMPLE_RATE
    tv.traceMode = 'always'
  })
  after(function (done) {
    emitter.close(done)
  })

  it('should construct', function () {
    new addon.UdpReporter('127.0.0.1')
  })

  it('should report event', function (done) {
    var event = addon.Context.createEvent()

    // Receive the message from the udp server and verify the id matches
    emitter.once('message', function (msg) {
      msg.should.have.property('X-Trace', event.toString())
      done()
    })

    emitter.reporter.sendReport(event)
  })
})
