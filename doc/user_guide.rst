User Guide
==========

Operation
---------

Operations can be used to define functionalities whose implementations differ
among devices.

This is done by extending the Operation class and implementing the overloads
of the Operation::run() method for all Executor types. When invoking the
Executor::run() method with the Operation as input, the library will select
the Operation::run() overload corresponding to the dynamic type of the
Executor instance.

Consider an overload of ``operator<<`` for Executors, which prints some basic
device information (e.g. device type and id) of the Executor to a C++ stream::

  std::ostream& operator<<(std::ostream &os, const gko::Executor &exec);

One possible implementation would be to use RTTI to find the dynamic type of
the Executor, However, using the Operation feature of Ginkgo, there is a
more elegant approach which utilizes polymorphism. The first step is to
define an Operation that will print the desired information for each Executor
type.::

  class DeviceInfoPrinter : public gko::Operation {
  public:
      explicit DeviceInfoPrinter(std::ostream &os) : os_(os) {}

      void run(const gko::OmpExecutor *) const override { os_ << "OMP"; }

      void run(const gko::CudaExecutor *exec) const override
      { os_ << "CUDA(" << exec->get_device_id() << ")"; }

      void run(const gko::HipExecutor *exec) const override
      { os_ << "HIP(" << exec->get_device_id() << ")"; }

      void run(const gko::DpcppExecutor *exec) const override
      { os_ << "DPC++(" << exec->get_device_id() << ")"; }

      // This is optional, if not overloaded, defaults to OmpExecutor overload
      void run(const gko::ReferenceExecutor *) const override
      { os_ << "Reference CPU"; }

  private:
      std::ostream &os_;
  };

Using DeviceInfoPrinter, the implementation of ``operator<<`` is as simple as
calling the ``run()`` method of the executor.::

  std::ostream& operator<<(std::ostream &os, const gko::Executor &exec)
  {
      DeviceInfoPrinter printer(os);
      exec.run(printer);
      return os;
  }

Now it is possible to write the following code:::

  auto omp = gko::OmpExecutor::create();
  std::cout << *omp << std::endl
            << *gko::CudaExecutor::create(0, omp) << std::endl
            << *gko::HipExecutor::create(0, omp) << std::endl
            << *gko::DpcppExecutor::create(0, omp) << std::endl
            << *gko::ReferenceExecutor::create() << std::endl;

which produces the expected output:

.. code-block:: text

  OMP
  CUDA(0)
  HIP(0)
  DPC++(0)
  Reference CPU

One might feel that this code is too complicated for such a simple task.
Luckily, there is an overload of the ``Executor::run()`` method, which is
designed to facilitate writing simple operations like this one. The method
takes four closures as input: one which is run for OMP, one for CUDA
executors, one for HIP executors, and the last one for DPC++ executors. Using
this method, there is no need to implement an Operation subclass:::

  std::ostream& operator<<(std::ostream &os, const gko::Executor &exec)
  {
      exec.run(
          [&]() { os << "OMP"; },  // OMP closure
          [&]() { os << "CUDA("    // CUDA closure
                     << static_cast<gko::CudaExecutor&>(exec)
                          .get_device_id()
                     << ")"; },
          [&]() { os << "HIP("    // HIP closure
                     << static_cast<gko::HipExecutor&>(exec)
                          .get_device_id()
                     << ")"; });
          [&]() { os << "DPC++("    // DPC++ closure
                     << static_cast<gko::DpcppExecutor&>(exec)
                          .get_device_id()
                     << ")"; });
      return os;
  }

Using this approach, however, it is impossible to distinguish between
a OmpExecutor and ReferenceExecutor, as both of them call the OMP closure.