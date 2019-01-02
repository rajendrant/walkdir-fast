path = require 'path'
assert = require 'assert'
walkdir = require '../'
mm = require('micromatch')
util = require './util'
{performance} = require 'perf_hooks'

time_walkdir_fast = 0
time_js_fs_api = 0

test = (dir, options={}) ->
  time = performance.now()
  actual = walkdir.sync dir, JSON.parse(JSON.stringify(options))
  time_walkdir_fast += performance.now() - time
  time = performance.now()
  expected = util.walkdirExpected dir, options
  time_js_fs_api += performance.now() - time
  actual.sort()
  expected.sort()
  it JSON.stringify(options), ->
    assert.deepEqual actual, expected, 'test failed for dir and options'

testAsync = (dir, options={}) ->
  it JSON.stringify(options), (done) ->
    @timeout 10000
    actual = []
    time = performance.now()
    expected = util.walkdirExpected dir, options
    time_js_fs_api += performance.now() - time
    time = performance.now()
    walkdir dir, JSON.parse(JSON.stringify(options))
    .on 'file', (fname) ->
      actual.push fname
    .on 'end', ->
      time_walkdir_fast += performance.now() - time
      actual.sort()
      expected.sort()
      assert.deepEqual actual, expected, 'test failed for dir and options'
      done()

util.createFilesForTest 'chrome'
testpath = path.join 'benchmark', 'data', 'chrome'

describe 'test the files returned are the same with the javascript version', ->
  describe 'with various glob pattern options', ->
    test testpath + path.sep + 'browser'
    test testpath + path.sep + 'browser' + path.sep
    test testpath, ignored_globs: ['DEPS']
    test testpath, ignored_globs: ['DEPS', 'OWNERS']
    test testpath, ignored_globs: ['DEPS', 'test']
    test testpath, ignored_globs: ['*.cc', '*.h']
    test testpath, ignored_globs: ['BUILD.gn']
    test testpath, ignored_globs: ['**'+path.sep+'test'+path.sep+'base'+path.sep+'**']
    test testpath, ignored_globs: ['**'+path.sep+'test'+path.sep+'!(base)'+path.sep+'**']
    it 'total time taken', ->
      console.log "\n\nTotal time taken for non-gitignore tests"
      console.log "walkdir-fast #{time_walkdir_fast} ms"
      console.log "nodejs FS api, #{time_js_fs_api} ms"

describe 'test the async mode', ->
  describe 'with various glob pattern options', ->
    testAsync testpath + path.sep + 'browser'
    testAsync testpath + path.sep + 'browser' + path.sep
    testAsync testpath, ignored_globs: ['DEPS']
    testAsync testpath, ignored_globs: ['DEPS', 'OWNERS']
    it 'total time taken', ->
      console.log "\n\nTotal time taken for gitignore tests"
      console.log "walkdir-fast #{time_walkdir_fast} ms"
      console.log "nodejs FS api, #{time_js_fs_api} ms"

describe 'test the git ignore options', ->
  describe 'with various glob pattern options', ->
    test testpath + path.sep + 'browser', skip_gitignored: true
