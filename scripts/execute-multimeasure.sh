#!/bin/sh

cd ../userspace/simple-ruby-director
ruby cli/CliClient.rb "multimeasure planFilename 'plans/${1}' path 'plans'"
