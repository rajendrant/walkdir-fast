path = require 'path'
EventEmitter = require('events').EventEmitter
binding = require('node-gyp-build')(__dirname)
mm = require('micromatch')

# http://www.delorie.com/djgpp/doc/incs/dirent.h

parseOptions = (options) ->
  options.follow_symlinks ?= false
  options.ignored_globs ?= []
  options.sync ?= false
  return options

isNotGlobPattern = (str) ->
  Array.from('*.+?![]()').every (char) => !str.includes(char)

class WalkDirFast extends EventEmitter
  constructor: (dir, @options) ->
    super()
    ignored_names = []
    ignored_start_names = []
    @ignored_globs = []
    @allfiles = []
    for ignore in @options.ignored_globs
      if isNotGlobPattern ignore
        ignored_names.push ignore
      else if ignore.slice(-1) is '*' && isNotGlobPattern ignore.slice(0,-1)
        ignored_start_names.push ignore.slice(0,-1)
      else
        @ignored_globs.push ignore
    dir = dir.slice(0, -1) if dir.slice(-1) == path.sep
    @obj =new binding.WalkDir dir, @options.follow_symlinks, ignored_names, ignored_start_names
    if @options.sync
      @GetNextFileEntries()
    else
      process.nextTick => @GetNextFileEntries()

  GetNextFileEntries: ->
    [fnames, types] = @obj.GetNextFileEntries()
    index = 0
    while index < fnames.length
      file = fnames[index]
      ignore = @ignored_globs && mm.any file, @ignored_globs, {matchBase: true, dot: true}
      if ignore
        while index+1 < fnames.length
          break unless fnames[index+1].startsWith file+path.sep
          index++
      else if types[index]!=0x4
        @emit 'file', file
        @allfiles.push file
      index++
    @emit 'end', @allfiles

module.exports = walkdir = (dir, options={}) ->
  options = parseOptions(options)
  new WalkDirFast dir, options

walkdir.sync = (dir, options={}) ->
  options = parseOptions(options)
  options.sync = true
  obj = new WalkDirFast dir, options
  return obj.allfiles
