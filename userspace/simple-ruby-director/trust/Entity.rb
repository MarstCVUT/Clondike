require 'set'

# Helper class respresenting a relation implied by a certificate
class CertifiedRelation
    # Certificate claiming the relation
    attr_reader :certificate    
    # Related object (another entity, delegation,...)
    attr_reader :object
    
    def initialize(object, certificate)
        @object = object
        @certificate = certificate
    end
    
    def eql?(other)
      @object == other.object
    end

    def hash()
        @object.hash
    end
end

module CompositeEntity
    # Hash of entities that compose this entity
    # Key: Entity
    # Value: CertifiedRelation
    attr_reader :entities    
    
    def addEntity(entity, certificate)
        @entities[entity] = CertifiedRelation.new(entity, certificate)
        entity.setContainedIn(self)
    end
    
    def removeEntity(entity)
        @entities.delete(entity)
        entity.removeContainedIn(self)
    end    
    
    def eachEntity() 
       @entities.each { |certifiedRelation|
           yield certifiedRelation.object
       }     
    end
    
    # Finds and returns contained subentity by name
    # Returns nil if there is no matching subentity
    def subentity(entityName)
        @entities.each_value { |entityRelation|
            return entityRelation if ( entityRelation.object.name == entityName)
        }
        return nil
    end
end

# Entity, that can be referenced in ACLs
class Entity
    # Name of entity
    attr_reader :name        
    # Set of Entity objects, that refer (are composed of) this Entity object
    attr_reader :containedIn
    # Set of ACLs this entity is authorized to (does not include delegations set, which as well implies authorization!)
    # Represented by a CertifiedRelation
    attr_reader :authorizations
    # Set of delegation rights assigned to this entity
    # Represented by a CertifiedRelation
    attr_reader :delegations
    
    def initialize(name)
        @name = name
        @containedIn = Set.new
        @authorizations = Set.new
        @delegations = Set.new
    end
    
    def setContainedIn(entity)
        #puts "SELF NAME: #{object_id} .. class #{self.class} .. #{name}"
        #puts "Setting contains in #{@containedIn.size}"
        @containedIn.add(entity)
        #puts "Setting contains in #{@containedIn.size}  AFTER "
    end
    
    def removeContainedIn(entity)
        @containedIn.delete(entity)
    end
    
    def addAuthorization(accessControlList, certificate)
        @authorizations.add(CertifiedRelation.new(accessControlList,certificate))
        accessControlList.addAuthorizedEntity(self)
    end
    
    def removeAuthorization(accessControlList, certificate)
        @authorizations.delete(CertifiedRelation.new(accessControlList,certificate))
        accessControlList.removeAuthorizedEntity(self)
    end
    
    def addDelegation(delegation, certificate)
        @delegations.add(CertifiedRelation.new(delegation,certificate))
        delegation.addEntity(self)
    end
    
    def removeDelegation(delegation, certificate)
        @delegations.delete(CertifiedRelation.new(delegation,certificate))
        delegation.removeEntity(self)
    end
    
    def authorizationsWithDelegations()
        result = Set.new
        @authorizations.each { |authorizationRelation|
            result.add(authorizationRelation.object)
        }
        
        @delegations.each { |delegationRelation|
            result.add(delegationRelation.object.accessControlList())
        }
        return result
    end
    
    def to_s
        "Type #{self.class} Name #{name}. Contained id #{containedIn}"
    end

    def eql?(other)
      self == other
    end

    def == (other)
      #puts "COMPARING .. other #{other.class}"
      return false if other == nil
      return false if (!other.kind_of?(Entity))
      #puts "#{other}"      
      #puts "#{self.class} #{other.class} #{self.class == other.class}"
      #puts "#{name} #{other.name} #{name == other.name}"
      #puts "#{name == other.name && self.class == other.class}"
      return name == other.name && self.class == other.class
    end

    def hash
      #puts "#{self} -> Hash: #{name.hash}"
      name.hash
    end
end

# This entity is a special "atomic" entity, that has a globally valid
# unique name (for example public key).
class GlobalEntity<Entity
    def initialize(name)
        super(name)
    end
end

# Entity of a node. Name is the nodes public key
class NodeEntity<GlobalEntity
    def initialize(name)
        super(name)
    end

    def ==(other)
      #puts "CMP #{other}"
      return false if other == nil
      super(other)
    end

end

# Class used for reference counting, holding an associated certificate related with the reference
class CertifiedReferenceCountTracker
    # Certification of the relation
    attr_reader :certifice
    # Count of references
    attr_reader :referenceCount
    
    def initialize(certificate)
        @certificate = certificate
        @referenceCount = 1
    end
    
    def addReference()
        @referenceCount = @referenceCount + 1
    end
    
    def decReference()
        @referenceCount = @referenceCount - 1
    end    
    
    def isReferenced()
        return (@referenceCount != nil and @referenceCount > 0)
    end
end

# Group of nodes declared by some remote node (i.e. not locally)
# Represented by a public key of the remote node concated with the group name
# The group name is a local name defined on the remote node
class RemoteGroupEntity<GlobalEntity
    # Public key of the entity where the local group name is valid
    attr_reader :scope
    # Certifier reference tracking
    attr_reader :referenceCounter
    
    include CompositeEntity
    
    def initialize(name, scope)
        super(name)
        @scope = scope        
        @entities = {}
        #@referenceCounter = 
    end

    def registerInterest(certificate)
        raise "Already initialized" if ( @referenceCounter != nil )
        @referenceCounter = CertifiedReferenceCountTracker.new(certificate)
    end
    
    def deregisterInterest(certificate)
        raise "Invalid deregistration request" if ( @referenceCounter == nil || @referenceCounter.certificate != certificate )
        @referenceCounter = nil
    end
    
    def ==(other)
      return false if other == nil
      super(other) && self.scope == other.scope
    end

    def to_s
      super.to_s + " Scope: '#{scope}'"
    end
end

# Locally defined entity name. Can be composed of one or more other identities
# both global and local
class LocalEntity<Entity    
    include CompositeEntity
    
    def initialize(name)
        super(name)
        @entities = {}
    end        
        
    def to_s
        "#{super.to_s} Contains: #{@entities.keys}"
    end
end

class LocalNodeEntity<LocalEntity
    def initialize(name)
        super(name)
    end
end

class LocalGroupEntity<LocalEntity
    # Hash of entities interested in group membership changes
    # Key: ID of entity Value: Count of references
    attr_reader :interestedEntities
    
    def initialize(name)
        super(name)
        
        @interestedEntities = {}
    end
    
    def registerInterestedEntity(entity)
        currentReferences = @interestedEntities[entity.name]
        currentReferences = 0 if ( currentReferences == nil )
        currentReferences = currentReferences + 1
        @interestedEntities[entity.name] = currentReferences
    end
    
    def deregisterInterestedEntity(entity)
        currentReferences = @interestedEntities[entity.name]
        currentReferences = 0 if ( currentReferences == nil )
        currentReferences = currentReferences - 1
        @interestedEntities[entity.name] = currentReferences
    end    
    
    def isInterested(entity)
        return @interestedEntities[entity.name] != nil && @interestedEntities[entity.name] > 0
    end
    
    def eachInterestedKey() 
            @interestedEntities.each { |entityName, referenceCount|
                yield entityName if ( referenceCount > 0 )
            }
    end
end
