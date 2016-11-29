import ast
import lambdaparser
import types
import islpy as isl


CODE = '''
define x1 = 1;
assume opAddNat: (Nat,Nat)->Nat;
assume intakeLower10: Nat->Nat;
define x3 = opAddNat (1,2);
define x4 = (lambda y: Nat. y) 1;
define x4 = lambda y: Nat. (y, y);
define x5 = (lambda y: Nat. (y, y)) 1;
define x6 = (lambda y: Nat. 1);

define c1 = intakeLower10 0;
define c2 = intakeLower10 12;
define c3 = intakeLower10 (opAddNat (3, 5));
'''

CODE2 = '''
// Natural and floating-point numbers
define Nat = Nat;
assume Float: *;
assume opFloatPlus: (Float, Float) -> Float;
assume opFloatMult: (Float, Float) -> Float;
assume cFloatZero: Float;

// Arrays
assume ArrT: pi_:(Nat, Nat -> *). *;
assume ArrCreate: pi type: (Nat, Nat -> *). (pi i: Nat. type[1] i) -> ArrT type;
assume ArrGet: pi type: (Nat, Nat -> *). ArrT type -> pi i:Nat. type[1] i;
define UArrT = lambda lt: (Nat, *). ArrT (lt[0], lambda _:Nat. lt[1]);

//define MatrixType = lambda n:Nat. lambda m:Nat. ArrT (n, (lambda _:Nat. ArrT (m, (lambda _:Nat. Float))));
define MatrixType = lambda n:Nat. lambda m:Nat. UArrT (n, UArrT (m, Float));

assume reduce: pi t:*. pi op: ((t, t) -> t). pi startval:t. pi len:Nat. pi values:(Nat -> t). t;
define sum = reduce Float opFloatPlus cFloatZero;

// check this:
assume testarray1: ArrT (15, lambda _:Nat. Float);
define a1 = ArrGet (15, lambda _:Nat. Float) testarray1 0;
define a2 = ArrGet (15, lambda _:Nat. Float) testarray1 16;
define a3 = lambda n:Nat. ArrGet (n, lambda _:Nat. Float) testarray1 16;
define a4 = (lambda n:Nat. ArrGet (n, lambda _:Nat. Float) testarray1 16) 20;
'''


def type_manually():
	# manual typing
	ass = ast.get_assumption('opAddNat')
	ass.get_constraints = types.MethodType(ast.nat_add_constraint, ass)

	def intakeLower10_constraints(self):
		a = ast.get_var_name()
		space = isl.Space.create_from_names(ast.ctx, set=[a])
		possible = isl.BasicSet.universe(space)
		accepted = isl.BasicSet.universe(space)
		accepted = accepted.add_constraint(isl.Constraint.ineq_from_names(accepted.space, {1: 10, a: -1}))
		return ([a, a], accepted, possible)

	ass = ast.get_assumption('intakeLower10')
	if ass:
		ass.get_constraints = types.MethodType(intakeLower10_constraints, ass)

	def reduce_constraints(self):
		var_len = ast.get_var_name('len')
		var_i = ast.get_var_name('i')
		vars = [[], [[[[], []], []], [[], [[var_len], [[[var_i], []], []]]]]]
		space = isl.Space.create_from_names(ast.ctx, set=[var_len, var_i])
		possible = isl.BasicSet.universe(space)
		accepted = isl.BasicSet.universe(space)
		# 0 <= i <= (len-1)
		possible = possible.add_constraint(isl.Constraint.ineq_from_names(possible.space, {var_i: 1}))
		possible = possible.add_constraint(isl.Constraint.ineq_from_names(possible.space, {var_len: 1, 1: -1, var_i: -1}))
		return (vars, accepted, possible)

	ass = ast.get_assumption('reduce')
	if ass:
		ass.get_constraints = types.MethodType(reduce_constraints, ass)

	def arr_get_constraints(self):
		var_len = ast.get_var_name('len')
		var_type_index = ast.get_var_name('ti')
		var_index = ast.get_var_name('i')
		vars = [[[var_len], [[var_type_index], []]], [[], [[var_index], []]]]
		space = isl.Space.create_from_names(ast.ctx, set=[var_len, var_type_index, var_index])
		possible = isl.BasicSet.universe(space)
		accepted = isl.BasicSet.universe(space)
		# accept only 0 <= i <= (len-1)
		#accepted = possible.add_constraint(isl.Constraint.ineq_from_names(possible.space, {var_index: 1}))
		accepted = accepted.add_constraint(isl.Constraint.ineq_from_names(possible.space, {var_len: 1, 1: -1, var_index: -1}))
		possible = possible.add_constraint(isl.Constraint.ineq_from_names(possible.space, {var_type_index: 1}))
		possible = possible.add_constraint(isl.Constraint.ineq_from_names(possible.space, {var_len: 1, 1: -1, var_type_index: -1}))
		return (vars, accepted, possible)

	ass = ast.get_assumption('ArrGet')
	if ass:
		ass.get_constraints = types.MethodType(arr_get_constraints, ass)


	# type unnecessary stuff
	for x in ['Float']:
		ass = ast.get_assumption(x)
		if ass:
			ass.get_constraints = types.MethodType(ast.empty_constraints, ass)





# add array stuff
#with open('arrays.lbl', 'r') as f:
#	arraycode = f.read()
#	arraycode = '\n'.join(arraycode.split('\n')[:18])
#	CODE += arraycode
prog = lambdaparser.parse_lambda_code(CODE+CODE2)
print prog
nodes = prog.to_ast()

type_manually()

for root in nodes:
	print '- ',root,
	print ': ',root.get_type()
	constraints = root.get_constraints()
	print 'Constraints: ', constraints
	print root.check_constraints(constraints)
	print ''




def test_typing():
	with open('arrays.lbl', 'r') as f:
		arraycode = f.read()
	prog = lambdaparser.parse_lambda_code(arraycode)
	nodes = prog.to_ast()[:-1] # skip matrix dot for now
	type_manually()

	for root in nodes:
		print '- ', root,
		print ': ', root.get_type()
		constraints = root.get_constraints()
		print 'Constraints: ', constraints
		print root.check_constraints()
		print ''
#test_typing()