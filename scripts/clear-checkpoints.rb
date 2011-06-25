#!/usr/bin/ruby

require 'fileutils'

# This script is for cases when rm -rf does not work.. it fails, when too many checkpoints accumlate - it crashes with too many arguments error

Dir.foreach("/home/clondike/") do |f|
  fullName = "/home/clondike/#{f}"  

  next if f =~ /^\..?/ # Exclude . and ..
  next if File.directory?(fullName)  
  FileUtils.rm(fullName)
end
