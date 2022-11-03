const path = require('path');
const fs = require('fs');

const map_from_array_of_tuples = (array, tuple_key_index) => {
	let map = {}
	for (const tuple of array) {
		let key = tuple[tuple_key_index];
		if (key === undefined) throw new Error("no key");
		if (map[key] !== undefined) throw new Error("duplicate key: " + key);
		map[key] = tuple;
	}
	return map;
};

const select = (targets) => {
	const target_map = map_from_array_of_tuples(targets, 0);
	const PRG = path.basename(process.argv[1]);
	const argv = process.argv.slice(2);

	if (argv.length !== 1) {
		console.log("Usage: " + PRG + " <target>");
		console.log("Targets:");
		for (const tuple of targets) {
			console.log("  " + tuple[0] + "\t\t(" + tuple[2] + ")");
		}
		process.exit(1);
	}

	const target_key = argv[0];
	const target = target_map[target_key];
	if (target === undefined) {
		console.log("Invalid target: " + target_key);
		process.exit(1);
	}

	return target;
};

const enter_target_dir = (target, prefix) => {
	const dir_to_enter = "_"+prefix+"_data_for_" + target[0];
	try { fs.mkdirSync(dir_to_enter); } catch(e) {}
	process.chdir(dir_to_enter);
	console.log("Dir: " + process.cwd() + " ...");
};

const assert = (p,msg) => { if (!p) throw new Error("ASSERTION FAILED (" + (msg ? msg : "nomsg") + ")"); };

exports.map_from_array_of_tuples = map_from_array_of_tuples;
exports.select = select;
exports.enter_target_dir = enter_target_dir;
exports.assert = assert;
