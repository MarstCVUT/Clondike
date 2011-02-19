#!/bin/sh

cd ../userspace/simple-ruby-director
ruby cli/CliClient.rb "measure resultFilename 'plans/${1}-result' planFilename 'plans/${1}'"