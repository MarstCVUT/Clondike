require 'trust/Delegation.rb'

# Root class representing an access control list
class AccessControlList
    # Set of Permission objects associated with this access control list
    attr_reader :permissions
    # Set of "Entity" objects associated to this access control list
    attr_reader :authorizedEntities
    # Set of "Delegation" objects associated with this ACL
    attr_reader :delegations
    # Entity, where is this authorization valid
    attr_reader :target
    
    def initialize(target)
        @permissions = Set.new
        @authorizedEntities = Set.new
        @delegations = Set.new
        @target = target
    end
    
    def addNewDelegation()
        delegation = Delegation.new(self, true, true, 60)
        @delegations.add(delegation)
        return delegation
    end
    
    def addAuthorizedEntity(entity)
        @authorizedEntities.add(entity)        
    end

    def removeAuthorizedEntity(entity)
        @authorizedEntities.delete(entity)        
    end
    
    def addPermission(permission)
        @permissions.add(permission)
    end
end