# Listener pluggable to TaskRepository, that can trace execution times of individual tasks
class ExecutionTimeTracer
    def newTask(task)
        # Nothing to be done
    end
    
    def taskExit(task, exitCode)
        endTime = Time.now.to_f
        puts  "Task #{task.name} (#{task.pid}) was executed in #{endTime-task.startTime} seconds. Exit code: #{exitCode}. Execution node: #{task.executionNode}"
    end
end