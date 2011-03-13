require 'ConfigurablePatternMatcher'

#This class can be registered as a listener to NetlinkConnector on npm events
#in order to dump all executions of executables matching the preconfigured pattern
class ExecDumper    
    def initialize
        loadPatterns("dump.patterns")
    end
    
    def onExec(pid, uid, name, is_guest, args=nil, envp=nil, rusage=nil)
        matching = matchesPattern(name)
        return nil if !matching        
        return [DirectorNetlinkApi::REQUIRE_ARGS_AND_ENVP] if !args
        
        dump(pid, uid, name, args, envp)
        nil
    end    
private    
    include ConfigurablePatternMatcher
    
    def dump(pid, uid, name, args, envp)        
        puts "========= Dump of #{name} [Pid: #{pid} - Uid: #{uid}] =============="
        puts "Args:"
        args.each { |arg| puts arg }
        puts "Envs:"
        envp.each { |env| puts env }
        puts "========= End of #{name} [#{pid}] =============="
        puts ""
    end    
end
