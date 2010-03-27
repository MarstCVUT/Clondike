require 'Util.rb'

# Permission to do everything
class AnyPermission
    def implies(permission)
        # Implies all permissions
        true
    end
end

# Permissions to file system
class FileSystemPermission
    # read, write, readwrite
    attr_accessor :permission
    # Path to which is the permission granted
    attr_accessor :path
    
    def initialize(permission, path)
        @permission = permission
        @path = path
        #puts "#{permission} ...... #{path}"
    end
    
    def self.parseFromConfigLine(line)
        matches = line.match(/fs (read|write|readwrite) (.+)/)
        raise "Syntax error at line '#{line}" if !matches
        
        return FileSystemPermission.new(matches[1], matches[2])
    end
    
    def implies(permission)
        return false if (!permission.kind_of?(FileSystemPermission))
        # check, if our permission (read, write, readwrite) is superset of second permission
        # it is enough to check second string is contained in first
        return false if ( !(@permission =~ /#{permission.permission}/) )
        #puts "P3 .. #{permission.path} vs .. #{@path} .... #{(permission.path.match(/^#{@path}/))} vs #{(@path.match(/^#{permission.path}/))}"
        # Permitted path must start with our path        
        # TODO: Check for ".." hacks!!
        return permission.path.startsWith(@path)
    end
    
    def toCommandLine()
        "fs #{@permission} #{path}"
    end
end

# Returns true, if each of check permissions is implied by at least one granted permission
def areAllPermissionImplied(grantedPermissions, checkPermissions)
    allImplied = true
    checkPermissions.each { |permission|
        allImplied = false
        grantedPermissions.each { |grantedPermission|
            #puts "???? #{grantedPermission.toCommandLine} implies #{permission.toCommandLine}"
            if ( grantedPermission.implies(permission)) then
                allImplied = true
                #puts "#{grantedPermission.toCommandLine} implies #{permission.toCommandLine}"
                #return true
            end
        }
        return false if !allImplied
    }
    return allImplied
end

def parsePermissionLine(line)
    return FileSystemPermission.parseFromConfigLine(line) if line.match(/^fs /)
    return AnyPermission.new if line.match(/^any/)

    raise "Unknown permission type on line '#{line}'"
end
