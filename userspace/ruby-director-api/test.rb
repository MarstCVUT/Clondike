require "directorApi"

#include DirectorNetlinkApi

def npmCallbackFunction pid, name, is_guest
	#puts "Hueee we are in callback"
	puts "Test callback called #{pid} name #{name} is_guest #{is_guest}" 
	#args.each { |arg| puts "Arg #{arg}" }
	#envs.each { |env| puts "Env #{env}" }
	binaryName = name.split("/").last
	puts binaryName

	if ( binaryName == "dmesg" )
		[DirectorNetlinkApi::REQUIRE_ENVP]
	else
		[DirectorNetlinkApi::DO_NOT_MIGRATE]
	end
end

def npmFullCallbackFunction pid, name, is_guest, args, envp
	#puts "Hueee we are in callback"
	puts "Test callback called #{pid} name #{name} is_guest #{is_guest}" 
	args.each { |arg| puts "Arg #{arg}" }
	envp.each { |env| puts "Env #{env}" }

	[DirectorNetlinkApi::DO_NOT_MIGRATE]
end

netlinkThread = Thread.new {
	DirectorNetlinkApi.instance.registerNpmCallback(:npmCallbackFunction)
	DirectorNetlinkApi.instance.registerNpmFullCallback(:npmFullCallbackFunction)
	DirectorNetlinkApi.instance.runProcessingLoop
}

puts "Waiting for netlink thread end"

netlinkThread.join
