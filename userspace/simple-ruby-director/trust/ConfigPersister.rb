require 'trust/Entity.rb'
require 'trust/Permission.rb'

# Helper class used for persisting of configuration
class ConfigPersister
    def initialize(entities, acls)
        @entities = entities
        @acls = acls
    end
    
    def save(fileName)
        File.open(fileName, "w") { |file|            
            file.puts("---")
            persistEntities(file)
            persistAcls(file)
        }        
    end
    
private
    def entityToString(entity)
        case entity
          when LocalEntity
              return entity.name
          when NodeEntity
              return "\{type: hash, value: #{entity.name}}"
          when RemoteGroupEntity
              return "\{type: remoteGroup, name: #{entity.name}, remoteId: #{entityToString(entity.scope)}}"
        end
    end
    
    def entitySetToString(entities)
        result = ""
        result = "[" if (entities.size > 1)

        entitiesCopy = Set.new(entities)
        entitiesCopy.collect!() { |element| "#{entityToString(element)}"}
        result += entitiesCopy.to_a.join(", ")
        
        result += "]" if (entities.size > 1)
        return result
    end
    
    def persistEntities(file)
        file.puts(" NameBindings:")

        @entities.each_value { |entity|
            # Only local entities are explicitly bound
            next if !entity.kind_of?(LocalEntity) 

            file.puts("  #{entity.name}: #{entitySetToString(entity.identities)}")
        }        
    end
    
    def persistAcls(file)
        file.puts(" ACLs:")

        @acls.each { |acl|
            file.puts("  -")
            file.puts("   Permissions:")
            acl.permissions.each { |permission|
                    file.puts("      #{permission.toCommandLine}")
            }
            file.puts("   AuthorizedUsers: #{entitySetToString(acl.authorizedEntities)}")
            file.puts("   Delegations:")
            acl.delegations.each { |delegation|
                file.puts("     -")
                file.puts("      Users: #{entitySetToString(delegation.authorizedEntities)}")
                file.puts("      PersistDelegations: #{delegation.persistDelegations}")
                file.puts("      EagerUpdate: #{delegation.eagerUpdate}")
                file.puts("      PollInterval: #{delegation.pollInterval}")
            }
        }        
    end    
end