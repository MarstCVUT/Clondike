#!/usr/bin/ruby

require 'fileutils'

Dir.foreach("/home/clondike/") do |f|
  fullName = "/home/clondike/#{f}"  

  next if f =~ /^\..?/ # Exclude . and ..
  next if File.directory?(fullName)
  puts "DELETING #{fullName}"
  FileUtils.rm(fullName)
end
