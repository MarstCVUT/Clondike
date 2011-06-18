# This is just a specific limiter for make testing
class TestMakeAcceptLimiter        
    def initialize()
        # At the moment we support only one parallel make run
        @currentMakePid = nil
        @singleThreadedPhase = false
        @makeExecutablePattern = Regexp.new("make$")
    end
    
    # Returns maximum number of tasks that can be yet accepted (i.e. in addition to already accepted tasks)
    def maximumAcceptCount
        return 100 if !makeRunning # Basically no limit
        return 2 if singleThreadedMakeRunning # Small limit when we are running only non-parallel make part
        # Do no accept anything while make is running in paralleled part
        return 0
    end

    # Callback from task repository
    def newTask(task)
        # TODO: Or some regexp
        if ( task.name =~ @makeExecutablePattern)            
            if ( @currentMakePid )
                # Just ignore this, we'll wait for the first make to finish
                #raise "New make started, but we do not support this!"
                return
            end
            
            #puts "Make running"
            @currentMakePid = task.pid 
            
        #TODO: This does not work ;( ld is not executed directly and the parameter vmlinux is not passed in args.. too bad   
        elsif ( @currentMakePid && task.name =~ /ld$/ && !@singleThreadedPhase )
            task.args.each() { |arg|
                if ( arg =~ /vmlinux1/ )                    
                    @singleThreadedPhase = true
                    return
                end
            }            
            puts "Started single threaded phase"
        end
    end
    
    # Callback from task repository
    def taskExit(task, exitCode)    
        if ( task.pid == @currentMakePid )
          #puts "Make done"
          @currentMakePid = nil
          @singleThreadedPhase = false
        end
    end
    

private    
        
    def makeRunning
        @currentMakePid != nil
    end
    
    def singleThreadedMakeRunning
        # TODO: Detect this case.. at least final linking belongs here
        false
    end
    
end