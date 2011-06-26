#!/bin/sh

killall ruby
sleep 1
cd ../userspace/simple-ruby-director
nice -20 ruby Director.rb > /tmp/director.log 2>&1 &