package com.example;

import org.apache.commons.csv.CSVFormat;
import org.apache.commons.csv.CSVParser;
import org.apache.commons.csv.CSVRecord;

import java.io.IOException;
import java.io.Reader;
import java.nio.file.Files;
import java.nio.file.Paths;
import java.text.ParseException;
import java.text.SimpleDateFormat;
import java.util.*;
import java.util.function.Predicate;
import java.util.logging.Level;
import java.util.logging.Logger;
import java.util.stream.Collectors;

public class EmployeeProcessor {

    private static final String CSV_FILE_PATH = "employees.csv";
    private static final Logger LOGGER = Logger.getLogger(EmployeeProcessor.class.getName());

    private static final String[] HEADERS = {
            "EmployeeID", "FirstName", "LastName", "Email", "PhoneNumber", "DateOfBirth", "Salary", "Department", "DateOfJoining"
    };

    public static void main(String[] args) {
        List<Employee> employees = readCSV(CSV_FILE_PATH);
        if (employees.isEmpty()) {
            System.out.println("No valid employee data found.");
        } else {
            printEmployeeDetails(employees);
            printAverageSalaryByDepartment(employees);
            printFilteredEmployees(employees, e -> e.getSalary() > 75000);
        }
    }

    public static List<Employee> readCSV(String filePath) {
        try (Reader reader = Files.newBufferedReader(Paths.get(filePath));
             CSVParser csvParser = new CSVParser(reader, CSVFormat.DEFAULT.withHeader(HEADERS).withFirstRecordAsHeader())) {

            return csvParser.getRecords().stream()
                    .map(EmployeeProcessor::parseEmployeeSafe)
                    .filter(Objects::nonNull)
                    .collect(Collectors.toList());

        } catch (IOException e) {
            LOGGER.log(Level.SEVERE, "Error reading CSV file", e);
            return Collections.emptyList();
        }
    }

    private static Employee parseEmployeeSafe(CSVRecord record) {
        try {
            return parseEmployee(record);
        } catch (Exception e) {
            LOGGER.log(Level.WARNING, "Skipping invalid record: " + record.toString(), e);
            return null;
        }
    }

    public static Employee parseEmployee(CSVRecord record) throws ParseException {
        int employeeID = Integer.parseInt(record.get("EmployeeID"));
        String firstName = record.get("FirstName");
        String lastName = record.get("LastName");
        String email = record.get("Email");
        String phoneNumber = record.get("PhoneNumber");
        SimpleDateFormat dateFormat = new SimpleDateFormat("yyyy-MM-dd");
        Date dateOfBirth = dateFormat.parse(record.get("DateOfBirth"));
        int salary = Integer.parseInt(record.get("Salary")); // Corrected line
        String department = record.get("Department");
        Date dateOfJoining = dateFormat.parse(record.get("DateOfJoining"));

        return new Employee(employeeID, firstName, lastName, email, phoneNumber, dateOfBirth, salary, department, dateOfJoining);
    }

    public static void printEmployeeDetails(List<Employee> employees) {
        System.out.println("Employee Details:");
        System.out.println("------------------------------------------------------------------------------");
        System.out.printf("%-6s | %-10s | %-10s | %-24s | %-8s | %-10s | %-8s | %-12s | %-12s%n",
                "ID", "First Name", "Last Name", "Email", "Phone", "DOB", "Salary", "Department", "Joining Date");
        System.out.println("------------------------------------------------------------------------------");
        SimpleDateFormat dateFormat = new SimpleDateFormat("yyyy-MM-dd");
        employees.forEach(employee -> System.out.printf("%-6d | %-10s | %-10s | %-24s | %-8s | %-10s | %-8.2f | %-12s | %-12s%n",
                employee.getEmployeeID(),
                employee.getFirstName(),
                employee.getLastName(),
                employee.getEmail(),
                employee.getPhoneNumber(),
                dateFormat.format(employee.getDateOfBirth()),
                employee.getSalary(),
                employee.getDepartment(),
                dateFormat.format(employee.getDateOfJoining())));
        System.out.println("------------------------------------------------------------------------------");
    }

    public static void printAverageSalaryByDepartment(List<Employee> employees) {
        Map<String, Double> averageSalaryByDepartment = employees.stream() // Corrected line
                .collect(Collectors.groupingBy(
                        Employee::getDepartment,
                        Collectors.averagingInt(Employee::getSalary)
                ));

        System.out.println("\nAverage Salary by Department:");
        averageSalaryByDepartment.forEach((department, averageSalary) ->
                System.out.printf("%-12s: $%.2f%n", department, averageSalary));
    }

    public static void printFilteredEmployees(List<Employee> employees, Predicate<Employee> filter) {
        List<Employee> filteredEmployees = employees.stream()
                .filter(filter)
                .collect(Collectors.toList());

        if (filteredEmployees.isEmpty()) {
            System.out.println("\nNo employees match the filter criteria.");
        } else {
            System.out.println("\nFiltered Employees:");
            System.out.println("------------------------------------------------------------------------------");
            System.out.printf("%-6s | %-10s | %-10s | %-24s | %-8s | %-10s | %-8s | %-12s | %-12s%n",
                    "ID", "First Name", "Last Name", "Email", "Phone", "DOB", "Salary", "Department", "Joining Date");
            System.out.println("------------------------------------------------------------------------------");
            SimpleDateFormat dateFormat = new SimpleDateFormat("yyyy-MM-dd");
            filteredEmployees.forEach(employee -> System.out.printf("%-6d | %-10s | %-10s | %-24s | %-8s | %-10s | %-8.2f | %-12s | %-12s%n",
                    employee.getEmployeeID(),
                    employee.getFirstName(),
                    employee.getLastName(),
                    employee.getEmail(),
                    employee.getPhoneNumber(),
                    dateFormat.format(employee.getDateOfBirth()),
                    employee.getSalary(),
                    employee.getDepartment(),
                    dateFormat.format(employee.getDateOfJoining())));
            System.out.println("------------------------------------------------------------------------------");
        }
    }
}
