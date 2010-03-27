# Helper class used for loading of configuration
class ConfigLoader    
    def initialize(entities, acls)
        @entities = entities
        @acls = acls
    end
    
    def load(fileName)
        configData = YAML::load(File.read(fileName))        
        parseNames(configData['NameBindings'])
        #parseNames(configData['Groups'])
        parseAclEntries(configData['ACLs'])
    end

private
      def isCompositeName(name)
          name.kind_of?(Hash)
      end
    
      def isGlobalNodeName(name)
          isCompositeName(name) && name["type"] == 'hash'
      end

      def isGlobalGroupName(name)
          isCompositeName(name) && name["type"] == 'remoteGroup'
      end
      
      def isGlobalEntityName(name)
          isGlobalNodeName(name) || isGlobalGroupName(name)
      end
      
      def resolveLocalEntityName(name)
          @entities[name]
      end
      
      # Converts global name hash into a string
      def globalEntityNameConverter(nameHash)
            if ( isGlobalNodeName(nameHash) )
                return "Remote node with hash: #{nameHash['value']}";
            elsif ( isGlobalGroupName(nameHash))
                # TODO: Include as well name of remote node!!!
                return "Remote group of node with hash: Group name: #{nameHash['name']}";
            else
                raise "Unknown global node type"
            end
      end

      def resolveGlobalEntityName(hashName)
          name = globalEntityNameConverter(hashName)
          globalEntity = @entities[name]
          if ( globalEntity == nil )
              if ( isGlobalNodeName(hashName) )
                  globalEntity = NodeEntity.new(hashName["value"])
              elsif ( isGlobalGroupName(hashName))
                  scope = resolveEntityName(hashName["remoteId"])
                  return nil if scope == nil # If the cope entity cannot be resolved now, we need to delay resolving of this entity
                  globalEntity = RemoteGroupEntity.new(hashName["name"], scope)
              else
                  raise "Unknown global node type"
              end
              @entities[name] = globalEntity
          end
          
          globalEntity
      end
      
      def resolveEntityName(name)
          if isGlobalEntityName(name)
              return resolveGlobalEntityName(name)
          end
          
          return resolveLocalEntityName(name)
      end
      
      def isAllResolvable(values)
          resolvable = true
          
          values.each { |val| 
              if (resolveEntityName(val) == nil ) 
                 resolvable = false
                 return
              end
          }
          
          return resolvable
      end
      
      def checkNewLocalName(name)
          if @entities.has_key?(name)
              raise "Duplicate name defined: #{name}"
          end
      end

      
      def parseNames(names)
          unresolvedPairs = {}
          
          names.each_pair { |key, value|  
              raise "Global names are not supported at left hand side of bindings" if isGlobalEntityName(key)
                  
              lhsEntity = resolveEntityName(key)
              values = (value.kind_of?(Array) ? value : [value])
              if ( !isAllResolvable(values) )
                  unresolvedPairs[key] = value
                  next
              end
              
              lhs = resolveEntityName(key)
              if lhs == nil
                checkNewLocalName(key)
                lhs = LocalEntity.new(key) 
                @entities[key] = lhs
              end
              
              values.each { |name|
                  entity = resolveEntityName(name)
                  lhs.addEntity(entity)
              }              
          }          
          
          remainingUnresolvedPairs = unresolvedPairs.size
          if ( remainingUnresolvedPairs > 0) 
              if ( remainingUnresolvedPairs == names.size )
                  raise "There are unresolved names remaining! #{names.keys}"
              end
              
              parseNames(unresolvedPairs)
          end
      end

      def parseAclEntries(entries)
        entries.each { |value|
          parseAclEntry(value)
        }
      end

      def parseAclEntry(entry)
        acl = AccessControlList.new()
        @acls.add(acl)
        parseDelegation(acl, entry['Delegations'])
        parseAuthorizations(acl, entry['AuthorizedUsers'])
        parsePermissions(acl, entry['Permissions'])
        #puts "AU: #{entry['AuthorizedUsers']}"
      end

      def parsePermissions(acl, permissionsConfig)
        permissionsConfig.each { |item|
          puts "Permission #{item}"
          permission = parsePermissionLine(item)
          acl.addPermission(permission)
        }
      end

      def parseAuthorizations(acl, authorizationConfig)
        authorizationConfig.each { |item|
            entity = resolveEntityName(item)
            raise "Failed to resolve entity #{item}" if !entity
            acl.addAuthorizedEntity(entity)
        }
      end

      def parseDelegation(acl, delegationsConfig)
        delegationsConfig.each { |delegationConfig|
            delegation = acl.addNewDelegation()

            delegation.persistDelegations = delegationConfig['PersistDelegations'] if delegationConfig.has_key?('PersistDelegations')
            delegation.eagerUpdate = delegationConfig['EagerUpdate'] if delegationConfig.has_key?('EagerUpdate')
            delegation.pollInterval = delegationConfig['PollInterval'] if delegationConfig.has_key?('PollInterval')

            delegationConfig['Users'].each { |item|
                entity = resolveEntityName(item)
                raise "Failed to resolve entity #{item}" if !entity
                delegation.addEntity(entity)
            }            
        }
      end
end