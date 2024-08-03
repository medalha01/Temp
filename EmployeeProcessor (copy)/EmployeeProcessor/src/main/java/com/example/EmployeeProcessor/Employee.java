import java.util.Date;

public class Employee {
    private int employeeID;
    private String firstName;
    private String lastName;
    private String email;
    private String phoneNumber;
    private Date dateOfBirth;
    private double salary;
    private String department;
    private Date dateOfJoining;

    public Employee(int employeeID, String firstName, String lastName, String email, String phoneNumber,
                    Date dateOfBirth, double salary, String department, Date dateOfJoining) {
        this.employeeID = employeeID;
        this.firstName = firstName;
        this.lastName = lastName;
        this.email = email;
        this.phoneNumber = phoneNumber;
        this.dateOfBirth = dateOfBirth;
        this.salary = salary;
        this.department = department;
        this.dateOfJoining = dateOfJoining;
    }

    public int getEmployeeID() {
        return employeeID;
    }

    public String getFirstName() {
        return firstName;
    }

    public String getLastName() {
        return lastName;
    }

    public String getEmail() {
        return email;
    }

    public String getPhoneNumber() {
        return phoneNumber;
    }

    public Date getDateOfBirth() {
        return dateOfBirth;
    }

    public double getSalary() {
        return salary;
    }

    public String getDepartment() {
        return department;
    }

    public Date getDateOfJoining() {
        return dateOfJoining;
    }
}

