package com.example;

@FunctionalInterface
public interface EmployeeFilter {
    boolean filter(Employee employee);
}

