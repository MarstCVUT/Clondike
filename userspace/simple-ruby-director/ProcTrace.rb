class CollectStats
    def initialize(logname)
    	@execs = {}
    	@operations = {}
	@log = File.new("#{Director::LOG_DIR}/#{logname}", "w")
	@start = Time.now
    end
    
    def finalize
        @log.write("\n---== Operation statistics ==---\n")
        @log.write("Count   Operation\n")
	@operations.sort{|a, b| b[1] <=> a[1]}.each{|op, num| @log.write("%5d   %s\n" % [num, op])}
        @log.write("\n---== Exec call statistics ==---\n")
        @log.write("Execs   User time   Sys time   Image\n")
	@execs.sort{|a, b| b[1][0] <=> a[1][0]}.each{|image, num| @log.write("%5d   %9.03f   %8.03f   %s\n" % [num[0], num[1]/1000.0, num[2]/1000.0, image])}
	@log.close
    end

    def collect(actions, prefix)
	last_fork = actions.rindex{|a| a.class == LogFork}
	if last_fork then
	    last_fork = actions[last_fork]
	    sibling_indent = '|  '
	else
	    sibling_indent = '   '
	end

	actions.each do |a|
	    if a == last_fork then sibling_indent = '   ' end

	    if (a.class == LogExec) then 
                if (@execs.has_key?(a.image)) then
	            @execs[a.image][0] += 1
	            @execs[a.image][1] += a.user_time 
	            @execs[a.image][2] += a.sys_time 
	        else
	            @execs[a.image] = [1, a.user_time, a.sys_time]
	        end
	    end

	    if "#{a.class}" =~ /^Log/ then
	        op = "#{a.class}".sub(/^Log/, '')
                if @operations.has_key?(op) then
	            @operations[op] += 1
	        else
	            @operations[op] = 1
	        end
	    end

	    rtn = a.collect
	    if rtn[0] then
        	@log.write("%09.04f%s%s%s" % [(a.time - @start).to_f, prefix, (rtn[1] or sibling_indent), rtn[0]])
		if a.user_time then
		    @log.write(" U: #{a.user_time}ms")
		end
		if a.sys_time then
		    @log.write(" S: #{a.sys_time}ms")
		end
		@log.write("\n")
	    end
	    self.collect(rtn[2], prefix + sibling_indent)
	end
    end
end

#
# Logged actions
#
class Log
    attr_reader :time
    attr_reader :rusage
    attr_accessor :user_time
    attr_accessor :sys_time
    def initialize
    	@time = Time.now
	@rusage = nil
	@user_time = nil
	@sys_time = nil
    end

    def collect
        return [nil, nil, []]
    end
end

class LogExec < Log
    attr_reader :image
    attr_writer :outputTreeLine
    def initialize(image, rusage)
    	super()
	@image = image
	@outputTreeLine = true
	@rusage = rusage
    end

    def collect
	if @outputTreeLine
    	    return ["#{@image}", '', []]
	else
	    return [nil, nil, []]
	end
    end
end


class LogFork < Log
    def initialize(process)
    	super()
	@process = process
    end

    def collect
    	return @process.collect
    end

    def sys_time
        return @process.sys_time
    end

    def user_time
        return @process.user_time
    end
end

class LogExit < Log
    attr_reader :code
    def initialize(code, rusage)
    	super()
	@code = code
	@rusage = rusage
    end
end

#
# Process abstraction
#
class TracedProcess < Log
    attr_reader :currentImage
    def initialize(image, collector=nil)
    	super()
        @initialImage = image
        @currentImage = image
	@collector = collector
	@actions = []
    end

    def logAction(action)
        @actions.push(action)
	if (action.class == LogExit and @collector) then
	    @collector.collect([self], '')
	    @collector.finalize()
	end
	if (action.class == LogExec) then
	    @currentImage = action.image
	end
    end

    def collect
	# compute cpu_usage
	last_log = nil
	@actions.each do |a|
	    if a.rusage then
	    	if last_log then
		    last_log.user_time = (a.rusage['ru_utime']['tv_sec'] - last_log.rusage['ru_utime']['tv_sec']) * 1000 +
		                         (a.rusage['ru_utime']['tv_usec'] - last_log.rusage['ru_utime']['tv_usec']) / 1000.0
		    last_log.sys_time = (a.rusage['ru_stime']['tv_sec'] - last_log.rusage['ru_stime']['tv_sec']) * 1000 +
		                        (a.rusage['ru_stime']['tv_usec'] - last_log.rusage['ru_stime']['tv_usec']) / 1000.0
		else
		    @user_time = a.rusage['ru_utime']['tv_sec'] * 1000 + a.rusage['ru_utime']['tv_usec'] / 1000.0
		    @sys_time = a.rusage['ru_stime']['tv_sec'] * 1000 + a.rusage['ru_stime']['tv_usec'] / 1000.0
	        end
		last_log = a
	    end
	end

        if (@actions[0].class == LogExec) then # detect fork() and exec() cases
	    @actions[0].outputTreeLine = false
	    @user_time = @actions[0].user_time
	    @sys_time = @actions[0].sys_time
	    return [@actions[0].image, '+- ', @actions]
	else
	    return [@initialImage, '+- ', @actions]
	end
    end
end

class ProcTrace
    def initialize()
        @tasks = {}
    end

    # Callback on fork
    def onFork(pid, parentPid)
        if (@tasks.has_key?(parentPid)) then
	    @tasks[pid] = TracedProcess.new(@tasks[parentPid].currentImage)
	    @tasks[parentPid].logAction(LogFork.new(@tasks[pid]))
	end
    end
    
    # Callback from on exec notification
    def onExec(pid, uid, name, isGuest, args=nil, envp=nil, rusage=nil)
        if args == nil and envp == nil then
            if (@tasks.has_key?(pid)) then
	        @tasks[pid].logAction(LogExec.new(name, rusage))
    	    elsif (name == '/usr/bin/make') then
	        @tasks[pid] = TracedProcess.new(name, CollectStats.new(name.rpartition('/')[2] + Time.now.strftime("-%Y%m%d-%H%M%S-") + "#{pid}.log"))
	    end
	end
	nil        
    end
    
    # Callback on task exit
    def onExit(pid, exitCode, rusage)
        if (@tasks.has_key?(pid)) then
	    @tasks.delete(pid).logAction(LogExit.new(exitCode, rusage))
	end
    end
    
    # Callback in load balancer
    def onMigration(pid, response)
        # TODO: It is not guaranteed the migration will succeed at this time!
        return if !response
        
        # TODO: Ignoring migrate back decision
        return if response[0] != DirectorNetlinkApi::MIGRATE
    end
end