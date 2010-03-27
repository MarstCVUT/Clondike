require 'trust/Entity.rb'
require 'trust/AccessControlList.rb'
require 'trust/Delegation.rb'
require 'trust/Permission.rb'
require 'trust/ConfigLoader.rb'
require 'trust/ConfigPersister.rb'
require 'yaml'

# TODO: Remove this? Obsolote? Will not use files to map security, instead certificates are used for everything
# Security configuration
class SecurityConfiguration
    # Hash of all known entities (name->Entity). Note: Name can be as well a composite "hash" name
    attr_reader :entities
    # Set of all known ACLs
    attr_reader :acls
    
    def initialize()
        @entities = {}
        @acls = Set.new
    end
    
    def load(fileName)
        configLoader = ConfigLoader.new(entities, acls)
        configLoader.load(fileName)
    end
    
    def save(fileName)
        configPersister = ConfigPersister.new(entities, acls)
        configPersister.save(fileName)
    end    
end