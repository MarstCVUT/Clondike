require 'socket'  

class String
    def startsWith(prefix)
       self.match(/^#{prefix}/) ? true : false        
    end
    
    def endsWith(sufix)
       self.match(/#{sufix}$/) ? true : false        
    end    
end

class Hash
    # Returns single element contained in the hash. Raises error if there is different count of elements than 1
    def singleElement
        raise "Invalid element count: #{size}" if ( size != 1 )
        return to_a[0]
    end
end

class ExceptionAwareThread < Thread
    def initialize(*args)
     super do
	begin
           yield(*args)
	rescue => err
		$log.error "Error in thread: #{err.message} \n#{err.backtrace.join("\n")}"
	end       
     end
    end
end

# Returns local IP address (TODO: Works with 1 IP only)
def local_ip  
  orig, Socket.do_not_reverse_lookup = Socket.do_not_reverse_lookup, true  # turn off reverse DNS resolution temporarily  
  
  UDPSocket.open do |s|  
    s.connect '1.2.3.4', 1  
    s.addr.last  
  end  
ensure  
  Socket.do_not_reverse_lookup = orig  
end  

# Detects whether a passed IP is local (TODO: Cache local IP & make it working with multiple IPs)
def isLocalIp(ipAddress)
    ipAddress = ipAddress.last
    #puts "LOCAL IP TEST: #{local_ip()} vs #{ipAddress} => #{local_ip() == ipAddress}"
  return local_ip() == ipAddress
end

def formattedDuration(startTime, endTime)
  millisDiff = 1000*(endTime.to_f - startTime.to_f)
  millis = millisDiff%1000
  secondsDiff = millisDiff/1000
  seconds = secondsDiff%60
  minutes = secondsDiff/60
  return sprintf("%02d:%02d.%04d", minutes, seconds, millis)
end

def timedExecution(name)
  startTime = Time.now.to_f
  result = yield
  endTime = Time.now.to_f
  
  $log.debug "Execution of '#{name}' took: #{1000*(endTime - startTime)} ms"
  return result
end

class TimingProxy
    instance_methods.each { |m| undef_method m unless m =~ /(^__|^send$|^object_id$)/ }
    
    def initialize(object)
      @object = object
    end

    protected
        def method_missing(name, *args, &block)	  
	  timedExecution(name) {
	    @object.send(name, *args, &block)
	  }
        end
end